
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

// Shlick's approximation of Fresnel
float3 FresnelShlick(float3 F0, float3 F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}

float FresnelShlick(float F0, float F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * Pow5(1.0f - cosTheta);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}
//
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float KullaContyGeometry(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    
    float a2 = roughness * roughness;
    float GGX1 = 2.0 * NdotL / (NdotL + sqrt(a2 + (1.0 - a2) * NdotL * NdotL));
    float GGX2 = 2.0 * NdotV / (NdotV + sqrt(a2 + (1.0 - a2) * NdotV * NdotV));
    return GGX1 * GGX2;
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float3 roughnessFactor = 1.0 - roughness;
    return F0 + (max(roughnessFactor, F0) - F0) * Pow5(clamp(1.0 - cosTheta, 0.0, 1.0));
}

