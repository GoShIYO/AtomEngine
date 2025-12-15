#include"Utility.hlsli"

Texture2D ColorTex : register(t0);

float4 main(float4 position : SV_Position) : SV_Target0
{
    float4 color = ColorTex[(int2) position.xy];
    return float4(ApplySRGBCurve(color.rgb), color.a);
}