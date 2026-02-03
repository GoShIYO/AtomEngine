#include "PostEffectUtil.hlsli"

Texture2D<float3> Bloom : register(t0);
#if SUPPORT_TYPED_UAV_LOADS
RWTexture2D<float3> SrcColor : register( u0 );
#else
RWTexture2D<uint> DstColor : register(u0);
Texture2D<float3> SrcColor : register(t2);
#endif
RWTexture2D<float> OutLuma : register(u1);
SamplerState LinearSampler : register(s0);

cbuffer CB0 : register(b0)
{
    float2 gRcpBufferDim;
    float gBloomStrength;
};

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    float2 TexCoord = (DTid.xy + 0.5) * gRcpBufferDim;

    float3 ldrColor = SrcColor[DTid.xy] + gBloomStrength * Bloom.SampleLevel(LinearSampler, TexCoord, 0);

#if SUPPORT_TYPED_UAV_LOADS
    SrcColor[DTid.xy] = ldrColor;
#else
    DstColor[DTid.xy] = Pack_R11G11B10_FLOAT(ldrColor);
#endif
    OutLuma[DTid.xy] = RGBToLogLuminance(ldrColor);
}