cbuffer PerObject : register(b0)
{
    float4x4 World;
};

struct VS_IN
{
	float3 position : POSITION;
};

struct GS_IN
{
    float4 worldPos : WORLDPOS;
};

GS_IN main(VS_IN input)
{
    GS_IN output = (GS_IN)0;
    output.worldPos = mul(float4(input.position, 1.0), World);
    return output;
}
