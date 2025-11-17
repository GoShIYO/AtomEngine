cbuffer constans : register(b0)
{
    float4x4 viewProj;
};

struct VSInput
{
    float3 pos : POSITION;
    float4 color : COLOR0;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
};

VSOutput main(VSInput input)
{
    VSOutput o;
    o.pos = mul(float4(input.pos, 1),viewProj);
    o.color = input.color;
    return o;
}