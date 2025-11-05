#include "Common.hlsli"

cbuffer MeshConstants : register(b0)
{
    float4x4 WorldMatrix;
    float4x4 WorldIT;
};

cbuffer GlobalConstants : register(b1)
{
    float4x4 ViewProjMatrix;
    float4x4 SunShadowMatrix;
    float3 ViewerPos;
    float3 SunDirection;
    float3 SunIntensity;
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
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITTANGENT;
#ifdef ENABLE_SKINNING
    float4 jointWeights : WEIGHT;
    uint4 jointIndices : INDEX;
#endif
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    
    float3 worldPos : TEXCOORD1;
    float4 sunShadowCoord : TEXCOORD2;
};

VSOutput main(VSInput input)
{
    VSOutput vsOutput;

    float4 position = float4(input.position, 1.0f);
    float3 normal = input.normal;
    float3 tangent = input.tangent;
    float3 bitangent = input.bitangent;
    
#ifdef ENABLE_SKINNING
    float4 weights = input.jointWeights / dot(input.jointWeights, 1.0f);
    
    float4 skinnedPos = mul(position, Joints[input.jointIndices.x].PosMatrix) * weights.x;
    skinnedPos += mul(position, Joints[input.jointIndices.y].PosMatrix) * weights.y;
    skinnedPos += mul(position, Joints[input.jointIndices.z].PosMatrix) * weights.z;
    skinnedPos += mul(position, Joints[input.jointIndices.w].PosMatrix) * weights.w;
    position = skinnedPos;
    
    float3 skinnedNor = mul(normal, (float3x3) Joints[input.jointIndices.x].NrmMatrix) * weights.x;
    skinnedNor += mul(normal, (float3x3) Joints[input.jointIndices.y].NrmMatrix) * weights.y;
    skinnedNor += mul(normal, (float3x3) Joints[input.jointIndices.z].NrmMatrix) * weights.z;
    normal = skinnedNor;
    
    float3 skinnedTangent = mul(tangent.xyz, (float3x3) Joints[input.jointIndices.x].NrmMatrix) * weights.x;
    skinnedTangent += mul(tangent.xyz, (float3x3) Joints[input.jointIndices.y].NrmMatrix) * weights.y;
    skinnedTangent += mul(tangent.xyz, (float3x3) Joints[input.jointIndices.z].NrmMatrix) * weights.z;
    
    tangent.xyz = skinnedTangent;

    float3 skinnedBitangent = mul(bitangent.xyz, (float3x3) Joints[input.jointIndices.x].NrmMatrix) * weights.x;
    skinnedBitangent += mul(bitangent.xyz, (float3x3) Joints[input.jointIndices.y].NrmMatrix) * weights.y;
    skinnedBitangent += mul(bitangent.xyz, (float3x3) Joints[input.jointIndices.z].NrmMatrix) * weights.z;
    
    bitangent.xyz = skinnedBitangent;
    
#endif

    vsOutput.worldPos = mul(position, WorldMatrix).xyz;
    vsOutput.position = mul(float4(vsOutput.worldPos, 1.0), ViewProjMatrix);
    vsOutput.texcoord = input.texcoord;
    vsOutput.sunShadowCoord = mul(float4(vsOutput.worldPos, 1.0), SunShadowMatrix);
    vsOutput.normal = mul(normal, (float3x3) WorldIT);
    vsOutput.tangent = mul(tangent.xyz, (float3x3) WorldIT);
    vsOutput.bitangent = mul(bitangent.xyz, (float3x3) WorldIT);
    
    return vsOutput;
}