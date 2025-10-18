#include "Common.hlsli"

cbuffer MeshConstants : register(b0)
{
    float4x4 WorldMatrix;   // Object to world
    float3x3 WorldIT;       // Object normal to world normal
};

cbuffer GlobalConstants : register(b1)
{
    float4x4 ViewProjMatrix;
}

#ifdef ENABLE_SKINNING
struct Joint
{
    float4x4 PosMatrix;
    float4x4 NrmMatrix; // Inverse-transpose of PosMatrix
};

StructuredBuffer<Joint> Joints : register(t20);
#endif

struct VSInput
{
    float3 position : POSITION;
#ifdef ENABLE_ALPHATEST
    float2 uv : TEXCOORD0;
#endif
#ifdef ENABLE_SKINNING
    uint4 jointIndices : BLENDINDICES;
    float4 jointWeights : BLENDWEIGHT;
#endif
};

struct VSOutput
{
    float4 position : SV_POSITION;
#ifdef ENABLE_ALPHATEST
    float2 uv0 : TEXCOORD0;
#endif
};

VSOutput main(VSInput input)
{
    VSOutput vsOutput;

    float4 position = float4(input.position, 1.0);

#ifdef ENABLE_SKINNING
     float4 weights = input.jointWeights / dot(input.jointWeights, 1.0f);
    
    float4 skinnedPos = mul(position, Joints[input.jointIndices.x].PosMatrix) * weights.x;
    skinnedPos += mul(position, Joints[input.jointIndices.y].PosMatrix) * weights.y;
    skinnedPos += mul(position, Joints[input.jointIndices.z].PosMatrix) * weights.z;
    skinnedPos += mul(position, Joints[input.jointIndices.w].PosMatrix) * weights.w;
    position = skinnedPos;
#endif

    float3 worldPos = mul(position,WorldMatrix).xyz;
    vsOutput.position = mul(float4(worldPos, 1.0),ViewProjMatrix);

#ifdef ENABLE_ALPHATEST
    vsOutput.uv = input.uv;
#endif

    return vsOutput;
}
