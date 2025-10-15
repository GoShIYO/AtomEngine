#include "Common.hlsli"

Texture2D<float4> baseColorTexture : register(t0);
Texture2D<float3> metallicRoughnessTexture : register(t1);
Texture2D<float1> occlusionTexture : register(t2);
Texture2D<float3> emissiveTexture : register(t3);
Texture2D<float3> normalTexture : register(t4);

SamplerState baseColorSampler : register(s0);
SamplerState metallicRoughnessSampler : register(s1);
SamplerState occlusionSampler : register(s2);
SamplerState emissiveSampler : register(s3);
SamplerState normalSampler : register(s4);

TextureCube<float3> radianceIBLTexture : register(t10);
TextureCube<float3> irradianceIBLTexture : register(t11);
//Texture2D<float> texSSAO : register(t12);
Texture2D<float> texSunShadow : register(t13);

cbuffer MaterialConstants : register(b0)
{
    float4 baseColorFactor;
    float3 emissiveFactor;
    float normalTextureScale;
    float2 metallicRoughnessFactor;
    uint flags;
    float _pad0;
    float4x4 uvTransform;
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
    float2 texcoord0 : TEXCOORD0;
#ifndef NO_SECOND_UV
    float2 texcoord1 : TEXCOORD1;
#endif
    float3 normal : NORMAL;
#ifndef NO_TANGENT_FRAME
    float4 tangent : TANGENT;
#endif
    float3 worldPos : TEXCOORD2;
    float3 sunShadowCoord : TEXCOORD3;
};

// Numeric constants
static const float PI = 3.14159265;
static const float3 kDielectricSpecular = float3(0.04, 0.04, 0.04);

static const uint BASECOLOR = 0;
static const uint METALLICROUGHNESS = 1;
static const uint OCCLUSION = 2;
static const uint EMISSIVE = 3;
static const uint NORMAL = 4;
#ifndef HAS_UV1
#define UVSET( offset ) intput.texcoord0
#else
#define UVSET( offset ) lerp(intput.texcoord0, intput.texcoord1, (flags >> offset) & 1)
#endif

float2 GetUV(VSOutput intput,uint offset)
{
    float4 transformedUV = mul(float4(UVSET(offset), 0.0f, 1.0f), uvTransform);
    return transformedUV.xy;
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

//
// Shader Math
//

float Pow5(float x)
{
    float xSq = x * x;
    return xSq * xSq * x;
}

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
    return specular * radianceIBLTexture.SampleLevel(cubeMapSampler, reflect(-Surface.V, Surface.N), lod);
}

float3 ComputeTangentNormal(VSOutput input, float2 uv)
{
    float3 tangentNormal = normalTexture.Sample(normalSampler, uv).xyz * 2.0 - 1.0;

    float3 Q1 = ddx(input.worldPos);
    float3 Q2 = ddy(input.worldPos);
    float2 st1 = ddx(uv);
    float2 st2 = ddy(uv);

    float3 N = normalize(input.normal);
    float3 T = normalize(Q1 * st2.y - Q2 * st1.y);
    float3 B = -normalize(cross(T, N));
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(tangentNormal, TBN));
}

float3 ComputeNormal(VSOutput intput, float2 uv)
{
    float3 normal = normalize(intput.normal);

#ifdef NO_TANGENT_FRAME
    return normal;
#else
    float3 tangent = normalize(intput.tangent.xyz);
    float3 bitangent = normalize(cross(normal, tangent)) * intput.tangent.w;
    float3x3 tangentFrame = float3x3(tangent, bitangent, normal);

    normal = normalTexture.Sample(normalSampler, uv) * 2.0 - 1.0;
    
    normal = normalize(normal * float3(normalTextureScale, normalTextureScale, 1));

    return mul(normal, tangentFrame);
#endif
}

float CalcShadowFactor(float4 ShadowCoord)
{
    ShadowCoord.xyz /= ShadowCoord.w;

    float depth = ShadowCoord.z;
    
    uint width, height;
    texSunShadow.GetDimensions(width, height);
    
    float2 texelSize = 1.0 / float2(width, height);
    
    float shadow = 0.0f;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            float depth = texSunShadow.SampleCmpLevelZero(shadowSampler, ShadowCoord.xy + offset, ShadowCoord.z);
            shadow += depth;
        }
    }
    shadow /= 9.0f;

    return shadow;
}

float4 main(VSOutput input) : SV_Target0
{
    float4 baseColor = baseColorFactor * baseColorTexture.Sample(baseColorSampler, GetUV(input, BASECOLOR));
    float2 metallicRoughness = metallicRoughnessFactor *
        metallicRoughnessTexture.Sample(metallicRoughnessSampler, GetUV(input, METALLICROUGHNESS)).bg;
    float occlusion = occlusionTexture.Sample(occlusionSampler, GetUV(input, OCCLUSION));
    float3 emissive = emissiveFactor * emissiveTexture.Sample(emissiveSampler, GetUV(input, EMISSIVE));
    float3 normal = ComputeNormal(input, GetUV(input, NORMAL));

    SurfaceProperties Surface;
    Surface.N = normal;
    Surface.V = normalize(ViewerPos - input.worldPos);
    Surface.NdotV = saturate(dot(Surface.N, Surface.V));
    Surface.c_diff = baseColor.rgb * (1 - kDielectricSpecular) * (1 - metallicRoughness.x) * occlusion;
    Surface.c_spec = lerp(kDielectricSpecular, baseColor.rgb, metallicRoughness.x) * occlusion;
    Surface.roughness = metallicRoughness.y;
    Surface.alpha = metallicRoughness.y * metallicRoughness.y;
    Surface.alphaSqr = Surface.alpha * Surface.alpha;

    // Begin accumulating light starting with emissive
    float3 colorAccum = emissive;
    
    //colorAccum += Diffuse_IBL(Surface);
    //colorAccum += Specular_IBL(Surface);
    
    return float4(colorAccum, baseColor.a);
}