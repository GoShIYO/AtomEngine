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

// Diffuse IBL
float3 Diffuse_IBL(float3 diffuseAlbedo, float roughness,float3 N,float3 V)
{
    float3 irradiance = irradianceIBLTexture.Sample(anisotropicSampler, N).rgb;

    float LdotH_ibl = saturate(dot(N, normalize(N + V)));
    float fd90 = 0.5 + 2.0 * roughness * LdotH_ibl * LdotH_ibl;
    float3 diffuseFresnel = Fresnel_Schlick_F90(1.0, fd90, saturate(dot(N, V)));
    return diffuseAlbedo * irradiance * diffuseFresnel;
}
// Specular IBL
float3 Specular_IBL(float3 specularAlbedo, float3 V, float3 N, float roughness)
{
    float3 R = reflect(-V, N);
    float lod = saturate(roughness) * IBLRange + IBLBias;
    float3 prefiltered = radianceIBLTexture.SampleLevel(anisotropicSampler, R, lod).rgb;
    float3 F = Fresnel_Schlick(specularAlbedo, saturate(dot(N, V)));
    return prefiltered * F;
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
   
    float3 V = normalize(ViewerPos - input.worldPos);

    float3 F0 = lerp(FresnelF0, baseColor.rgb, metallic);

    float3 diffuseAlbedo = baseColor.rgb * (1.0 - metallic) * occlusion;

    float3 specularAlbedo = F0 * occlusion;

    roughness = saturate(roughness);
    float alpha = roughness * roughness;
    float alphaSqr = alpha * alpha;
    //太陽光(平行光源)
    float3 L = normalize(-SunDirection);
    float3 H = normalize(L + V);

    //dot
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));
    float LdotH = saturate(dot(L, H));

    //シャドウ
    float shadow = CalcShadowFactor(input.sunShadowCoord);

    // Diffuse Burley
    float3 diffuseTerm = Diffuse_Burley(diffuseAlbedo, roughness, NdotL, NdotV, LdotH);

    // Specular: D * G * F
    float D = D_GGX(NdotH, alphaSqr);
    // Schlick-GGX geometric
    float k = (roughness + 1.0);
    k = (k * k) / 8.0;
    float G = G_Smith(NdotV, NdotL, k);
    float3 F = Fresnel_Schlick(specularAlbedo, LdotH);

    float3 specularTerm = (D * G) * F;
    //直接光
    float3 dirLight = NdotL * SunIntensity * (diffuseTerm + specularTerm) * shadow;
    //間接光
    //拡散反射
    float3 diffuseIBL = Diffuse_IBL(diffuseAlbedo,roughness,N,V);
    //鏡面反射
    float3 specularIBL = Specular_IBL(specularAlbedo, V, N, roughness);
    
    float3 color = emissive + dirLight + diffuseIBL + specularIBL;

    return float4(color, baseColor.a);
}