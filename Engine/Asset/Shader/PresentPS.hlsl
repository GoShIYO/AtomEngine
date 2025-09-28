#include"Unility.hlsli"

Texture2D<float3> ColorTex : register(t0);

float4 main(float4 position : SV_Position) : SV_Target0
{
    float3 LinearRGB = ColorTex[(int2) position.xy];
    return float4(ApplySRGBCurve(LinearRGB), 1.0f);
}