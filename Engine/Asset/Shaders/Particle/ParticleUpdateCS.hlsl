#include "ParticleUtil.hlsli"

RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<uint> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
cbuffer EmitContants : register(b1)
{
    float3 translate;
    float radius;
    uint count;
    float frequency;
    float frequencyTime;
    uint emit;
}
[numthreads(256, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint index = DTid.x;
    if (index >= count)
        return;

    if (gParticles[index].currentTime >= gParticles[index].lifeTime || gParticles[index].lifeTime == 0)
    {
        if (emit == 0)
        {
            uint freeListIndex;
            InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);
            if ((freeListIndex + 1) < count)
            {
                gFreeList[freeListIndex + 1] = index;
            }
            else
            {
                InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
            }
        }
    }
    else
    {
        gParticles[index].position += gParticles[index].velocity * deltaTime;
        gParticles[index].currentTime += deltaTime;
        float alpha = 1.0 - (gParticles[index].currentTime / gParticles[index].lifeTime);
        gParticles[index].color.a = saturate(alpha);
    }
}