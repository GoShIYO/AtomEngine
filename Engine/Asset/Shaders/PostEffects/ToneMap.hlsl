#include "PostEffectUtil.hlsli"

StructuredBuffer<float> Exposure : register(t0);
Texture2D<float3> Bloom : register(t1);
RWTexture2D<float3> ColorRW : register(u0);
RWTexture2D<float> OutLuma : register(u1);
SamplerState LinearSampler : register(s0);

cbuffer CB0 : register(b0)
{
    float2 gRcpBufferDim;
    float gBloomStrength;
};

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float2 TexCoord = (DTid.xy + 0.5) * gRcpBufferDim;

    float3 hdrColor = ColorRW[DTid.xy];

    hdrColor += gBloomStrength * Bloom.SampleLevel(LinearSampler, TexCoord, 0);
    hdrColor *= Exposure[0];

    ColorRW[DTid.xy] = hdrColor;
    OutLuma[DTid.xy] = RGBToLogLuminance(hdrColor);


    // SDR¤Ø¤Î¥È©`¥ó¥Þ¥Ã¥×
    float3 sdrColor = TM_Stanard(hdrColor);

    ColorRW[DTid.xy] = sdrColor;

    OutLuma[DTid.xy] = RGBToLogLuminance(sdrColor);

}