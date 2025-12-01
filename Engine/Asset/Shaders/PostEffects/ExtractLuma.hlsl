#include "PostEffectUtil.hlsli"

SamplerState BiLinearClamp : register(s0);
Texture2D<float3> SourceTex : register(t0);
StructuredBuffer<float> Exposure : register(t1);
RWTexture2D<uint> LumaResult : register(u0);

cbuffer cb0
{
    float2 ginverseOutputSize;
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    //4つのサンプルがカバーする象限のちょうど中央にくるように、スケール係数と1ピクセルのサイズが必要です。
    float2 uv = DTid.xy * ginverseOutputSize;
    float2 offset = ginverseOutputSize * 0.25f;

    //2倍以上縮小したときにアンダーサンプリングが発生しないように、4つのバイリニアサンプルを使用します。
    float3 color1 = SourceTex.SampleLevel(BiLinearClamp, uv + float2(-offset.x, -offset.y), 0);
    float3 color2 = SourceTex.SampleLevel(BiLinearClamp, uv + float2(offset.x, -offset.y), 0);
    float3 color3 = SourceTex.SampleLevel(BiLinearClamp, uv + float2(-offset.x, offset.y), 0);
    float3 color4 = SourceTex.SampleLevel(BiLinearClamp, uv + float2(offset.x, offset.y), 0);

    //平均輝度を計算する
    float luma = RGBToLuminance(color1 + color2 + color3 + color4) * 0.25;

    //log(0)を防止し、Histogram[0]に純粋な黒ピクセルのみを配置します
    if (luma == 0.0)
    {
        LumaResult[DTid.xy] = 0;
    }
    else
    {
        const float MinLog = Exposure[4];
        const float RcpLogRange = Exposure[7];
        float logLuma = saturate((log2(luma) - MinLog) * RcpLogRange); //[0.0, 1.0]
        LumaResult[DTid.xy] = logLuma * 254.0 + 1.0; //[1, 255]
    }
}