// cbuffer VSConstants : register(b0)
// {
//     float4x4 modelToProjection;
// };

// struct VertexIn
// {
//     float3 PosL : POSITION;
//     float4 Color : COLOR;
// };

// struct VertexOut
// {
//     float4 PosH : SV_POSITION;
//     float4 Color : COLOR;
// };

// VertexOut VSMain(VertexIn vin)
// {
//     VertexOut vout;

//     // Transform to homogeneous clip space.
//     vout.PosH = mul(float4(vin.PosL, 1.0),modelToProjection);

//     // Just pass vertex color into the pixel shader.
//     vout.Color = vin.Color;

//     return vout;
// }

// float4 PSMain(VertexOut pin) : SV_Target0
// {
//     return pin.Color;
// }

cbuffer VSConstants : register(b0)
{
    float4x4 worldView;
    float3x3 worldInvT;
}

struct VertexIn {
    float3 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT0;
    float3 bitTangent : TANGENT1;
};

struct VertexOut {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT0;
    float3 bitTangent : TANGENT1;
};

VertexOut VSMain(VertexIn vin) {
    VertexOut output;
    output.position = mul(float4(vin.position, 1.0), worldView);
    output.uv = vin.texcoord;
    output.normal = mul(vin.normal, worldInvT);
    output.tangent = mul(vin.tangent, worldInvT);
    output.bitTangent = mul(vin.bitTangent, worldInvT);
    return output;
}

Texture2D<float4> baseColorTexture : register(t0);
Texture2D<float3> metallicRoughnessTexture : register(t1);
Texture2D<float3> normalTexture : register(t2);
Texture2D<float1> occlusionTexture : register(t3);
Texture2D<float3> emissiveTexture : register(t4);

SamplerState linearSamp : register(s0);

float4 PSMain(VertexOut pin) : SV_Target0 {
    float4 baseColor = baseColorTexture.Sample(linearSamp, pin.uv);
    return baseColor;
}
