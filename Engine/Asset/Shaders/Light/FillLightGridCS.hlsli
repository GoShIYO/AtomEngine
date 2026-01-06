#include "LightGrid.hlsli"
#define FLT_MIN         1.175494351e-38F        // min positive value
#define FLT_MAX         3.402823466e+38F        // max value
#define PI				3.1415926535f
#define TWOPI			6.283185307f

cbuffer CSConstants : register(b0)
{
    float4x4 ProjMatrix;
    float4x4 InvProjMatrix;
    float4x4 ViewMatrix;
    uint ViewportWidth, ViewportHeight;
    uint TileCountX;
    float NearClip;
    float FarClip;
};

StructuredBuffer<LightData> lightBuffer : register(t0);
Texture2D<float> depthTex : register(t1);
RWByteAddressBuffer lightGrid : register(u0);
RWByteAddressBuffer lightGridBitMask : register(u1);

groupshared uint minDepthUInt;
groupshared uint maxDepthUInt;

groupshared uint tileLightCountSphere;
groupshared uint tileLightCountCone;
groupshared uint tileLightCountConeShadowed;

groupshared uint tileLightIndicesSphere[MAX_LIGHTS];
groupshared uint tileLightIndicesCone[MAX_LIGHTS];
groupshared uint tileLightIndicesConeShadowed[MAX_LIGHTS];

groupshared uint4 tileLightBitMask;

#define _RootSig \
    "RootFlags(0), " \
    "CBV(b0), " \
    "DescriptorTable(SRV(t0, numDescriptors = 2))," \
    "DescriptorTable(UAV(u0, numDescriptors = 2))"

void GetTileFrustumPlane(out float4 frustumPlanes[6], uint3 Gid)
{
    float minTileZ = asfloat(minDepthUInt);
    float maxTileZ = asfloat(maxDepthUInt);

    float2 tileScale = float2(ViewportWidth, ViewportHeight) * rcp(float(2 * TILE_WIDTH));
    float2 tileBias = tileScale - float2(Gid.xy);

    float4 c1 = float4(ProjMatrix._11 * tileScale.x, 0.0, tileBias.x, 0.0);
    float4 c2 = float4(0.0, -ProjMatrix._22 * tileScale.y, tileBias.y, 0.0);
    float4 c4 = float4(0.0, 0.0, 1.0, 0.0);

    frustumPlanes[0] = c4 - c1; // Right
    frustumPlanes[1] = c4 + c1; // Left
    frustumPlanes[2] = c4 - c2; // Top
    frustumPlanes[3] = c4 + c2; // Bottom
    frustumPlanes[4] = float4(0.0, 0.0, 1.0, -minTileZ);
    frustumPlanes[5] = float4(0.0, 0.0, -1.0, maxTileZ);

    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        frustumPlanes[i] *= rcp(length(frustumPlanes[i].xyz));
    }
}

float3 ComputePositionInCamera(uint2 globalCoords)
{
    float2 st = ((float2) globalCoords + 0.5) * rcp(float2(ViewportWidth, ViewportHeight));
    st = st * float2(2.0, -2.0) - float2(1.0, -1.0);
    float3 screenPos;
    screenPos.xy = st.xy;
    screenPos.z = depthTex.Load(uint3(globalCoords, 0.0f));
    float4 cameraPos = mul(float4(screenPos, 1.0f),InvProjMatrix);

    return cameraPos.xyz / cameraPos.w;
}

[RootSignature(_RootSig)]
[numthreads(TILE_WIDTH, TILE_HEIGHT, 1)]
void CSMain(
   uint3 Gid : SV_GroupID,
    uint3 DTid : SV_DispatchThreadID,
    uint3 GTid : SV_GroupThreadID)
{
    uint GI = GTid.y * TILE_WIDTH + GTid.x;
    if (GI == 0)
    {
        tileLightCountSphere = 0;
        tileLightCountCone = 0;
        tileLightCountConeShadowed = 0;
        tileLightBitMask = 0;
        minDepthUInt = 0xffffffff;
        maxDepthUInt = 0;
    }
    uint2 frameUV = DTid.xy;

    float3 posInView = ComputePositionInCamera(frameUV);
    
    GroupMemoryBarrierWithGroupSync();
    
    if (DTid.x < ViewportWidth && DTid.y < ViewportHeight)
    {
        InterlockedMin(minDepthUInt, asuint(posInView.z));
        InterlockedMax(maxDepthUInt, asuint(posInView.z));
    }

    GroupMemoryBarrierWithGroupSync();
    
    float4 frustumPlanes[6];
    GetTileFrustumPlane(frustumPlanes, Gid);

    uint tileIndex = GetTileIndex(Gid.xy, TileCountX);
    uint tileOffset = GetTileOffset(tileIndex);

    // find set of lights that overlap this tile
    for (uint lightIndex = GI; lightIndex < MAX_LIGHTS; lightIndex += 64)
    {
        LightData lightData = lightBuffer[lightIndex];
        if (!lightData.isActive)
            continue;
        float4 lightViewPos = mul(float4(lightData.position, 1.0),ViewMatrix);
        float lightCullRadius = sqrt(lightData.radiusSq);

        bool overlapping = true;
        for (int p = 0; p < 6; p++)
        {
            float d = dot(frustumPlanes[p], lightViewPos);

            overlapping = overlapping && (d >= -lightCullRadius);
        }
        
        if (!overlapping)
            continue;

        uint slot;

        switch (lightData.type)
        {
            case 0: // sphere
                InterlockedAdd(tileLightCountSphere, 1, slot);
                tileLightIndicesSphere[slot] = lightIndex;
                break;

            case 1: // cone
                InterlockedAdd(tileLightCountCone, 1, slot);
                tileLightIndicesCone[slot] = lightIndex;
                break;

            case 2: // cone w/ shadow map
                InterlockedAdd(tileLightCountConeShadowed, 1, slot);
                tileLightIndicesConeShadowed[slot] = lightIndex;
                break;
        }

        // update bitmask
        InterlockedOr(tileLightBitMask[lightIndex / 32], 1u << (lightIndex % 32));
    }

    GroupMemoryBarrierWithGroupSync();

    if (GI == 0)
    {
        uint lightCount =
            ((tileLightCountSphere & 0xff) << 0) |
            ((tileLightCountCone & 0xff) << 8) |
            ((tileLightCountConeShadowed & 0xff) << 16);
        lightGrid.Store(tileOffset + 0, lightCount);

        uint storeOffset = tileOffset + 4;
        uint n;
        for (n = 0; n < tileLightCountSphere; n++)
        {
            lightGrid.Store(storeOffset, tileLightIndicesSphere[n]);
            storeOffset += 4;
        }
        for (n = 0; n < tileLightCountCone; n++)
        {
            lightGrid.Store(storeOffset, tileLightIndicesCone[n]);
            storeOffset += 4;
        }
        for (n = 0; n < tileLightCountConeShadowed; n++)
        {
            lightGrid.Store(storeOffset, tileLightIndicesConeShadowed[n]);
            storeOffset += 4;
        }

        lightGridBitMask.Store4(tileIndex * 16, tileLightBitMask);
    }
}

