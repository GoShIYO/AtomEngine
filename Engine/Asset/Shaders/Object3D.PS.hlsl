#include "Light/Lighting.hlsli"

Texture2D<float4> baseColorTexture : register(t0);
#ifdef USE_METALLICROUGHNESS
Texture2D<float3> metallicRoughnessTexture : register(t1);
Texture2D<float3> normalTexture : register(t2);
Texture2D<float1> occlusionTexture : register(t3);
Texture2D<float3> emissiveTexture : register(t4);
#else
Texture2D<float> metallicTexture : register(t1);
Texture2D<float> roughnessTexture : register(t2);
Texture2D<float3> normalTexture : register(t3);
Texture2D<float1> occlusionTexture : register(t4);
Texture2D<float3> emissiveTexture : register(t5);
#endif

TextureCube<float3> radianceIBLTexture : register(t10);
TextureCube<float3> irradianceIBLTexture : register(t11);
Texture2D<float2> brdfLUT : register(t12);
Texture2D<float> texSunShadow : register(t13);
Texture2D<float> texSSAO : register(t14);

cbuffer MaterialConstants : register(b0)
{
    float4 baseColorFactor;
    float3 emissiveFactor;
    float normalTextureScale;
    float3 FresnelF0;
    float2 metallicRoughnessFactor;
}

cbuffer GlobalConstants : register(b1)
{
    float4x4 ViewProj;
    float4x4 SunShadowMatrix;
    float3 ViewerPos;
    float3 SunDirection;
    float3 SunIntensity;
    float _pad1;
    float IBLRange;
    float IBLBias;
}

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    
    float3 worldPos : TEXCOORD1;
    float4 sunShadowCoord : TEXCOORD2;
};

float3 ComputeNormal(VSOutput intput, float2 uv)
{
    float3 N = normalize(intput.normal);
    float3 T = normalize(intput.tangent);
    float3 B = normalize(intput.bitangent);
    float3x3 TBN = float3x3(T, B, N);

    float3 normalTS = normalTexture.Sample(defaultSampler, uv) * 2.0 - 1.0;
    normalTS = normalize(normalTS * float3(normalTextureScale, normalTextureScale, 1));

    return mul(normalTS, TBN);
}

float CalcShadowFactor(float4 ShadowCoord)
{
    ShadowCoord.xyz /= ShadowCoord.w;

    if (ShadowCoord.x < 0.0 || ShadowCoord.x > 1.0 ||
        ShadowCoord.y < 0.0 || ShadowCoord.y > 1.0 ||
        ShadowCoord.z < 0.0 || ShadowCoord.z > 1.0)
    {
        return 1.0f;
    }

    uint width, height;
    texSunShadow.GetDimensions(width, height);
    float2 texelSize = 1.0f / float2(width, height);

    float sum = 0.0f;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            float cmp = texSunShadow.SampleCmpLevelZero(shadowSampler, ShadowCoord.xy + offset, ShadowCoord.z);
            sum += cmp;
        }
    }
    return sum / 9.0f;
}

struct SurfaceProperties
{
    float3 N;
    float3 V;
    float3 c_diff;
    float3 c_spec;
    float roughness;
    float alpha; // roughness squared
    float alphaSqr; // alpha squared
    float NdotV;
};

struct LightProperties
{
    float3 L;
    float NdotL;
    float LdotH;
    float NdotH;
};
// Shlick's approximation of Fresnel
float3 Fresnel_Shlick(float3 F0, float3 F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}

float Fresnel_Shlick(float F0, float F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}

// Burley's diffuse BRDF
float3 Diffuse_Burley(SurfaceProperties Surface, LightProperties Light)
{
    float fd90 = 0.5 + 2.0 * Surface.roughness * Light.LdotH * Light.LdotH;
    return Surface.c_diff * Fresnel_Shlick(1, fd90, Light.NdotL).x * Fresnel_Shlick(1, fd90, Surface.NdotV).x;
}

// GGX specular D (normal distribution)
float Specular_D_GGX(SurfaceProperties Surface, LightProperties Light)
{
    float lower = lerp(1, Surface.alphaSqr, Light.NdotH * Light.NdotH);
    return Surface.alphaSqr / max(1e-6, PI * lower * lower);
}

// Schlick-Smith specular geometric visibility function
float G_Schlick_Smith(SurfaceProperties Surface, LightProperties Light)
{
    return 1.0 / max(1e-6, lerp(Surface.NdotV, 1, Surface.alpha * 0.5) * lerp(Light.NdotL, 1, Surface.alpha * 0.5));
}

// Schlick-Smith specular visibility with Hable's LdotH approximation
float G_Shlick_Smith_Hable(SurfaceProperties Surface, LightProperties Light)
{
    return 1.0 / lerp(Light.LdotH * Light.LdotH, 1, Surface.alphaSqr * 0.25);
}


// A microfacet based BRDF.
// alpha:    This is roughness squared as in the Disney PBR model by Burley et al.
// c_spec:   The F0 reflectance value - 0.04 for non-metals, or RGB for metals.  This is the specular albedo.
// NdotV, NdotL, LdotH, NdotH:  vector dot products
//  N - surface normal
//  V - normalized view vector
//  L - normalized direction to light
//  H - normalized half vector (L+V)/2 -- halfway between L and V
float3 Specular_BRDF(SurfaceProperties Surface, LightProperties Light)
{
    // Normal Distribution term
    float ND = Specular_D_GGX(Surface, Light);

    // Geometric Visibility term
    //float GV = G_Schlick_Smith(Surface, Light);
    float GV = G_Shlick_Smith_Hable(Surface, Light);

    // Fresnel term
    float3 F = Fresnel_Shlick(Surface.c_spec, 1.0, Light.LdotH);

    return ND * GV * F;
}

float3 ShadeDirectionalLight(SurfaceProperties Surface, float3 L, float3 c_light)
{
    LightProperties Light;
    Light.L = L;

    // Half vector
    float3 H = normalize(L + Surface.V);

    // Pre-compute dot products
    Light.NdotL = saturate(dot(Surface.N, L));
    Light.LdotH = saturate(dot(L, H));
    Light.NdotH = saturate(dot(Surface.N, H));

    // Diffuse & specular factors
    float3 diffuse = Diffuse_Burley(Surface, Light);
    float3 specular = Specular_BRDF(Surface, Light);

    // Directional light
    return Light.NdotL * c_light * (diffuse + specular);
}

// Diffuse irradiance
float3 Diffuse_IBL(SurfaceProperties Surface)
{
    // Assumption:  L = N

    //return Surface.c_diff * irradianceIBLTexture.Sample(defaultSampler, Surface.N);

    // This is nicer but more expensive, and specular can often drown out the diffuse anyway
    float LdotH = saturate(dot(Surface.N, normalize(Surface.N + Surface.V)));
    float fd90 = 0.5 + 2.0 * Surface.roughness * LdotH * LdotH;
    float3 DiffuseBurley = Surface.c_diff * Fresnel_Shlick(1, fd90, Surface.NdotV);
    return DiffuseBurley * irradianceIBLTexture.Sample(defaultSampler, Surface.N);
}

// Approximate specular IBL by sampling lower mips according to roughness.  Then modulate by Fresnel. 
float3 Specular_IBL(SurfaceProperties Surface)
{
    float lod = Surface.roughness * IBLRange + IBLBias;
    float3 specular = Fresnel_Shlick(Surface.c_spec, 1, Surface.NdotV);
    return specular * radianceIBLTexture.SampleLevel(defaultSampler, reflect(-Surface.V, Surface.N), lod);
}

float4 main(VSOutput input) : SV_Target0
{
    
    #ifdef USE_METALLICROUGHNESS
    float4 baseColor = baseColorFactor * baseColorTexture.Sample(defaultSampler, input.texcoord);
    float metallic = metallicRoughnessFactor.x * metallicRoughnessTexture.Sample(defaultSampler, input.texcoord).b;
    float roughness = metallicRoughnessFactor.y * metallicRoughnessTexture.Sample(defaultSampler, input.texcoord).g;
    float occlusion = occlusionTexture.Sample(defaultSampler, input.texcoord);
    float3 emissive = emissiveFactor * emissiveTexture.Sample(defaultSampler, input.texcoord);
    float3 N = ComputeNormal(input, input.texcoord);
    #else
    float4 baseColor = baseColorFactor * baseColorTexture.Sample(defaultSampler, input.texcoord);
    float metallic = metallicRoughnessFactor.x * metallicTexture.Sample(defaultSampler, input.texcoord);
    float roughness = metallicRoughnessFactor.y * roughnessTexture.Sample(defaultSampler, input.texcoord);
    float occlusion = occlusionTexture.Sample(defaultSampler, input.texcoord);
    float3 emissive = emissiveFactor * emissiveTexture.Sample(defaultSampler, input.texcoord);
    float3 N = ComputeNormal(input, input.texcoord);
    #endif
    
     // 視線方向
    float3 V = normalize(ViewerPos - input.worldPos);
    // フレネル
    float3 F0 = lerp(FresnelF0, baseColor.rgb, metallic);
    float3 R = reflect(-V, N);
    //light color
    float3 Lo = emissive;
    float NdotV = saturate(dot(N, V));
    //太陽光(平行光源)
    {
        float3 L = normalize(-SunDirection);
        float3 H = normalize(L + V);
        float NdotL = saturate(dot(N, L));
        float LdotH = saturate(dot(L, H));
        float VdotH = saturate(dot(V, H));
        
        //GGX
        float NDF = DistributionGGX(N, H, roughness);
        //ジオメトリ関数
        float G = GeometrySmith(N, V, L, roughness);
        //フレネル
        float3 F = FresnelSchlick(VdotH, F0);
        
        float3 kS = F;
        float3 kD = 1.0f - kS;
        kD *= 1.0 - metallic;
        
        float3 numerator = NDF * G * F;
        float denom = max(4.0f * NdotV * NdotL, 1e-5f);
        float3 specular = numerator / denom;
        float3 diffuse = kD * baseColor.rgb / PI;

        float shadowFactor = CalcShadowFactor(input.sunShadowCoord);

        Lo += (diffuse + specular) * SunIntensity * NdotL;
    }
   
    float3 kS = fresnelSchlickRoughness(saturate(dot(N, V)), F0, roughness);
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    float3 irradiance = irradianceIBLTexture.Sample(anisotropicSampler, N).rgb;
    float lod = roughness * IBLRange + IBLBias;
    float3 prefilteredColor = radianceIBLTexture.SampleLevel(anisotropicSampler, R, lod).rgb;
    float2 brdf = brdfLUT.Sample(anisotropicSampler, float2(NdotV, roughness)).rg;

    //拡散反射
    float3 diffuseIBL = irradiance * baseColor.rgb;
    //鏡面反射
    float3 specularIBL = prefilteredColor * (F0 * brdf.x + brdf.y);

    //環境光
    float3 ambient = (kD * diffuseIBL + specularIBL) * occlusion;

    float3 output = ambient + Lo;

    return float4(output, baseColor.a);
}