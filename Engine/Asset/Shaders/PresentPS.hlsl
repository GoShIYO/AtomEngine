#include"Unility.hlsli"

Texture2D<float3> ColorTex : register(t0);

float3 TM_Reinhard(float3 hdr, float k = 1.0)
{
    return hdr / (hdr + k);
}

float3 TM_Stanard(float3 hdr)
{
    return TM_Reinhard(hdr * sqrt(hdr), sqrt(4.0 / 27.0));
}

float4 main(float4 position : SV_Position) : SV_Target0
{
    //TODO:post effect §À“∆Ñ”§π§Î
    float3 hdrColor = ColorTex[int2(position.xy)];
    float3 sdrColor = TM_Stanard(hdrColor);
    
    return float4(ApplySRGBCurve(sdrColor), 1.0f);
}