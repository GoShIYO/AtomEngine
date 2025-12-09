SamplerState defaultSampler : register(s10);
SamplerState anisotropicSampler : register(s11);
SamplerComparisonState shadowSampler : register(s12);

#define PI 3.14159265358979323f

//
// Shader Math
//
float Pow5(float x)
{
    float xSq = x * x;
    return xSq * xSq * x;
}

float3 Fresnel_Schlick(float3 F0, float cosine)
{
    return F0 + (1.0 - F0) * Pow5(1.0 - cosine);
}

// Fresnel-Schlick
float3 Fresnel_Schlick_F90(float3 F0, float3 F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}

float Fresnel_Schlick(float F0, float F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}

// GGX Normal Distribution D
float D_GGX(float NdotH, float alphaSqr)
{
    float denom = NdotH * NdotH * (alphaSqr - 1.0) + 1.0;
    denom = PI * denom * denom;
    return alphaSqr / max(1e-6, denom);
}

// Schlick-GGX geometric
float G_SchlickGGX_single(float NdotX, float k)
{
    return NdotX / max(1e-6, NdotX * (1.0 - k) + k);
}
float G_Smith(float NdotV, float NdotL, float roughness)
{
    float k = (roughness + 1.0);
    k = (k * k) / 8.0;
    return G_SchlickGGX_single(NdotV, k) * G_SchlickGGX_single(NdotL, k);
}

float3 Diffuse_Burley(float3 albedo, float roughness, float NdotL, float NdotV, float LdotH)
{
    float fd90 = 0.5 + 2.0 * roughness * LdotH * LdotH;
    float3 F_L = Fresnel_Schlick(1, fd90, NdotL);
    float3 F_V = Fresnel_Schlick(1, fd90, NdotV);
    return albedo * (F_L * F_V);
}

float fract(float x)
{
    return x - floor(x);
}
float2 fract(float2 x)
{
    return x - floor(x);
}

float3 fract(float3 x)
{
    return x - floor(x);
}

float rcp(float x)
{
    return 1.0 / x;
}

float2 rcp(float2 x)
{
    return float2(1.0, 1.0) / x;
}

float3 rcp(float3 x)
{
    return float3(1.0, 1.0, 1.0) / x;
}
