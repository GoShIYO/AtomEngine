#include "RayShadowUtil.hlsli"

[shader("anyhit")]
void ShadowAnyHit(inout RayPayload payload)
{
    payload.isHit = true;

    AcceptHitAndEndSearch();
}