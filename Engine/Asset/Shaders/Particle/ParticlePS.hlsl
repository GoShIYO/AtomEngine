#include"ParticleUtil.hlsli"

Texture2D<float4> gTexture : register(t1);
SamplerState gSampler : register(s0);

float4 main(VSOutput input) :SV_Target0
{
    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    return input.color * texColor;
}