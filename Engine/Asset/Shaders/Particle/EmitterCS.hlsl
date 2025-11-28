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

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{

    if (emit != 0)
    {
        for (uint i = 0; i < count; ++i)
        {
            int freeListIndex;
            InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
            
            if (0 <= freeListIndex && freeListIndex < MAX_PARTICLE)
            {
                uint index = gFreeList[freeListIndex];
                RandomGenerator rnd;
                rnd.seed = float3(index * 23.45, time, index * time * 11.78);

                gParticles[index].position = translate;
                float3 randVec = normalize(rnd.Generate3D() * 2.0 - 1.0);
                gParticles[index].velocity = randVec * 5.0f;
                gParticles[index].lifeTime = 2.0 * rnd.Generate1D() + 1.0;
                gParticles[index].currentTime = 0.0;
                gParticles[index].scale = float3(1, 1, 1);
                gParticles[index].color = float4(rnd.Generate3D(), 1);
            }
            else
            {
                InterlockedAdd(gFreeListIndex[0], 1);
                break;
            }
        }

    }
    
}