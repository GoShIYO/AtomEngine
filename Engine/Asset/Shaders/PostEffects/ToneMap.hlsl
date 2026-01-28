#include "PostEffectUtil.hlsli"

StructuredBuffer<float> Exposure : register(t0);
Texture2D<float3> Bloom : register(t1);
#if SUPPORT_TYPED_UAV_LOADS
RWTexture2D<float3> ColorRW : register( u0 );
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
    float PaperWhiteRatio; // PaperWhite / MaxBrightness
    float MaxBrightness;
};

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float2 TexCoord = (DTid.xy + 0.5) * gRcpBufferDim;

    // Load HDR and bloom
#if SUPPORT_TYPED_UAV_LOADS
    float3 hdrColor = ColorRW[DTid.xy];
#else
    float3 hdrColor = SrcColor[DTid.xy];
#endif

    hdrColor += gBloomStrength * Bloom.SampleLevel(LinearSampler, TexCoord, 0);
    hdrColor *= Exposure[0];

#if ENABLE_HDR_DISPLAY_MAPPING

    hdrColor = TM_Stanard(REC709toREC2020(hdrColor) * PaperWhiteRatio) * MaxBrightness;
    // Write the HDR color as-is and defer display mapping until we composite with UI
#if SUPPORT_TYPED_UAV_LOADS
    ColorRW[DTid.xy] = hdrColor;
#else
    DstColor[DTid.xy] = Pack_R11G11B10_FLOAT(hdrColor);
#endif
    OutLuma[DTid.xy] = RGBToLogLuminance(hdrColor);

#else

    // Tone map to SDR
    float3 sdrColor = TM_Stanard(hdrColor);

#if SUPPORT_TYPED_UAV_LOADS
    ColorRW[DTid.xy] = sdrColor;
#else
    DstColor[DTid.xy] = Pack_R11G11B10_FLOAT(sdrColor);
#endif

    OutLuma[DTid.xy] = RGBToLogLuminance(sdrColor);

#endif
}