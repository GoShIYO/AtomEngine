#include "RayShadowUtil.hlsli"

[shader("miss")]
void Miss(inout RayPayload payload)
{
    if (!payload.SkipShading)
    {
        gShadowMask[DispatchRaysIndex().xy] = float4(0, 0, 0, 1);
    }
}