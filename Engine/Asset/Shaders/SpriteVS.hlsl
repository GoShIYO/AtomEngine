cbuffer Constants : register(b0)
{
    float4x4 viewProj;
    float4x4 uvTransform;
    float4 color;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VSOutput main(float3 pos : POSITION, float2 uv : TEXCOORD)
{
    VSOutput output;
    
    output.pos = mul(float4(pos, 1.0), viewProj);
    output.uv = uv;
    
    return output;
}