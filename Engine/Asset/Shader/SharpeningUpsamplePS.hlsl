#include"Unility.hlsli"

Texture2D<float3> ColorTex : register(t0);
SamplerState BilinearClamp : register(s0);

cbuffer Constants : register(b0)
{
    float2 UVOffset0;   // Four UV offsets to determine the sharpening tap locations
    float2 UVOffset1;   // We get two more offsets by negating these two.
    float WA, WB;       // Sharpness strength:  WB = 1.0 + Sharpness; 4*WA + WB = 1
}

float3 GetColor(float2 UV)
{
    return ColorTex.SampleLevel(BilinearClamp, UV, 0);
}

float4 main(float4 position : SV_Position, float2 uv : TexCoord0) : SV_Target0
{
    float3 Color = WB * GetColor(uv) - WA * (
        GetColor(uv + UVOffset0) + GetColor(uv - UVOffset0) +
        GetColor(uv + UVOffset1) + GetColor(uv - UVOffset1));

    return float4(ApplySRGBCurve(Color), 1.0f);
}