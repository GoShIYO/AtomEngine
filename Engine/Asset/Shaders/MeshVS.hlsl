cbuffer cb : register(b0)
{
    float4x4 WorldMat;
    float4x4 WorldInvMat;
    float4x4 UVTransform;
    float4x4 ViewProjMat;
    float4 Color;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float4 color : COLOR0;
};

VSOutput main(VSInput input)
{
    VSOutput output;


    output.worldPos = mul(float4(input.position, 1.0f), WorldMat).xyz;
    output.pos = mul(float4(output.worldPos, 1.0f), ViewProjMat);

    float3 n = mul(input.normal, (float3x3)WorldInvMat);
    output.normal = normalize(n);

    output.uv = mul(float4(input.uv.x, input.uv.y, 0.0f, 1.0f), UVTransform).xy;
    
    output.color = Color;
    
    return output;
}