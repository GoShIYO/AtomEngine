#include "Common.hlsli"

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
//Texture2D<float> texSSAO : register(t12);
Texture2D<float> texSunShadow : register(t13);

cbuffer MaterialConstants : register(b0)
{
    float4 baseColorFactor;
    float3 emissiveFactor;
    float normalTextureScale;
    float2 metallicRoughnessFactor;
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
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bittangent : BITTANGENT;
    
    float3 worldPos : TEXCOORD1;
    float3 sunShadowCoord : TEXCOORD2;
};

float3 ComputeTangentNormal(VSOutput input, float2 uv)
{
    float3 tangentNormal = normalTexture.Sample(linearClamp, uv).xyz * 2.0 - 1.0;

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
    float3 tangent = normalize(intput.tangent);
    float3 bitangent = normalize(intput.bittangent);
    float3x3 tangentFrame = float3x3(tangent, bitangent, normal);

    normal = normalTexture.Sample(linearClamp, uv) * 2.0 - 1.0;
    
    normal = normalize(normal * float3(normalTextureScale, normalTextureScale, 1));

    return mul(normal, tangentFrame);
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
    
    #ifdef USE_METALLICROUGHNESS
    float4 baseColor = baseColorFactor * baseColorTexture.Sample(defaultSampler, input.texcoord);
    float2 metallicRoughness = metallicRoughnessFactor *
        metallicRoughnessTexture.Sample(defaultSampler, input.texcoord).bg;
    float occlusion = occlusionTexture.Sample(pointBorder, input.texcoord);
    float3 emissive = emissiveFactor * emissiveTexture.Sample(defaultSampler, input.texcoord);
    float3 normal = ComputeTangentNormal(input, input.texcoord);
    #else
    float4 baseColor = baseColorFactor * baseColorTexture.Sample(defaultSampler, input.texcoord);
    float metallic = metallicTexture.Sample(defaultSampler, input.texcoord);
    float roughness = roughnessTexture.Sample(defaultSampler, input.texcoord);
    float occlusion = occlusionTexture.Sample(pointBorder, input.texcoord);
    float3 emissive = emissiveFactor * emissiveTexture.Sample(defaultSampler, input.texcoord);
    float3 normal = ComputeTangentNormal(input, input.texcoord);
    #endif
    
    
    return baseColor;
}