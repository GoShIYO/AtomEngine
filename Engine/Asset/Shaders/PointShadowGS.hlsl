cbuffer PerFrame : register(b1)
{
    matrix ViewProj[6];
};

cbuffer PerObject : register(b2)
{
    float3 lightPos;
    float farPlane;
};

struct GS_IN
{
    float4 worldPos : WORLDPOS;
};

struct PS_IN
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLDPOS;
};

[maxvertexcount(18)]
void main(triangle GS_IN input[3], inout TriangleStream<PS_IN> stream)
{
    for (int f = 0; f < 6; ++f)
    {
        PS_IN output;
        for (int i = 0; i < 3; ++i)
        {
            output.worldPos = input[i].worldPos.xyz;
            output.position = mul(input[i].worldPos, ViewProj[f]);
            stream.Append(output);
        }
        stream.RestartStrip();
    }
}
