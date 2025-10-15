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
    float2 texcoord0 : TEXCOORD0;
    
#ifndef NO_SECOND_UV
    float2 texcoord1 : TEXCOORD1;
#endif
    float3 normal : NORMAL;
    
#ifndef NO_TANGENT_FRAME
    float4 tangent : TANGENT;
#endif
    
#ifdef ENABLE_SKINNING
    uint4 jointIndices : BLENDINDICES;
    float4 jointWeights : BLENDWEIGHT;
#endif
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord0 : TEXCOORD0;
#ifndef NO_SECOND_UV
    float2 texcoord1 : TEXCOORD1;
#endif
    float3 normal : NORMAL;
#ifndef NO_TANGENT_FRAME
    float4 tangent : TANGENT;
#endif
    float3 worldPos : TEXCOORD2;
    float3 sunShadowCoord : TEXCOORD3;
};

VSOutput main(VSInput input)
{
    VSOutput vsOutput;

    float4 position = float4(input.position, 1.0f);
    float3 normal = input.normal * 2 - 1;
#ifndef NO_TANGENT_FRAME
    float4 tangent = input.tangent;
#endif

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
#ifndef NO_TANGENT_FRAME
    float3 skinnedTangent = mul(tangent.xyz, (float3x3) Joints[input.jointIndices.x].NrmMatrix) * weights.x;
    skinnedTangent += mul(tangent.xyz, (float3x3) Joints[input.jointIndices.y].NrmMatrix) * weights.y;
    skinnedTangent += mul(tangent.xyz, (float3x3) Joints[input.jointIndices.z].NrmMatrix) * weights.z;
    
    tangent.xyz = skinnedTangent;
#endif

#endif

    vsOutput.worldPos = mul(position, WorldMatrix).xyz;
    vsOutput.position = mul(float4(vsOutput.worldPos, 1.0), ViewProjMatrix);
    vsOutput.sunShadowCoord = mul(float4(vsOutput.worldPos, 1.0), SunShadowMatrix).xyz;
    vsOutput.normal = mul(normal, (float3x3) WorldIT);
#ifndef NO_TANGENT_FRAME
    vsOutput.tangent = float4(mul(tangent.xyz, (float3x3) WorldIT), tangent.w);
#endif
    vsOutput.texcoord0 = input.texcoord0;
#ifndef NO_SECOND_UV
    vsOutput.texcoord1 = input.texcoord1;
#endif
    
    return vsOutput;
}