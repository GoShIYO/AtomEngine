cbuffer VSConstants : register(b0)
{
    float4x4 modelToProjection;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;

    // Transform to homogeneous clip space.
    vout.PosH = mul(float4(vin.PosL, 1.0),modelToProjection);

    // Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;

    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target0
{
    return pin.Color;
}
