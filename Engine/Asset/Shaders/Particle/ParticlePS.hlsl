#include"ParticleUtil.hlsli"

Texture2D<float4> gTextures[] : register(t1, space1);
SamplerState gSampler : register(s0);

float4 main(VSOutput input) :SV_Target0
{
    float4 texColor = gTextures[input.TexID].Sample(gSampler, input.texcoord);
    texColor.rgb *= texColor.a;
    
    return input.color * texColor;
}