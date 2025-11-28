#include "RayShadowUtil.hlsli"

[shader("raygeneration")]
void RayGen()
{
    uint2 dispatchIdx = DispatchRaysIndex().xy;

    float depth = gDepth.Load(int3(dispatchIdx, 0));

    if (depth >= 1.0f)
    {
        gShadowMask[dispatchIdx] = 1.0; // Not shadowed
        return;
    }

    float3 worldPosition = GetWorldPosition(dispatchIdx, depth);

    RayDesc rayDesc;
    rayDesc.Origin = worldPosition;
    rayDesc.Direction = gLightDir;
    rayDesc.TMin = SHADOW_RAY_EPSILON;
    rayDesc.TMax = FLT_MAX;

    RayPayload payload;
    payload.isHit = false;

    TraceRay(gAccel, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, ~0, 0, 1, 0, rayDesc, payload);


    gShadowMask[dispatchIdx] = payload.isHit ? 0.0f : 1.0f;
}