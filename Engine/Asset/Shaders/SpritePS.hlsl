cbuffer Constants : register(b0)
{
    float4x4 viewProj;
    float4x4 uvTransform;
    float4 color;
};

Texture2D<float4> texture : register(t0); // テクスチャ
sampler Sampler : register(s0);

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(VSOutput input) : SV_TARGET
{
    float4 uv = mul(float4(input.uv, 0.0f, 1.0f), uvTransform);
    
    float4 output = color * texture.Sample(Sampler, uv.xy);
    
    return output;
}