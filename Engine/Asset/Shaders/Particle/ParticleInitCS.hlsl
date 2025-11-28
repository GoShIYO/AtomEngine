#include"ParticleUtil.hlsli"

RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<uint> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

[RootSignature(Particle_RootSig)]
[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint index = DTid.x;
    if (index > MAX_PARTICLE)
        return;
    if (index == 0)
    {
        gFreeListIndex[0] = MAX_PARTICLE - 1;
    }
    RandomGenerator rng;
    rng.seed = float3(DTid);
    
    Particle p;
    p.scale = rng.Generate3D();
    p.position = rng.Generate3D();
    p.color = float4(rng.Generate3D(), rng.Generate1D());
    p.velocity = normalize(rng.Generate3D());
    p.lifeTime = rng.Generate1D();
    p.currentTime = 0.0;
    
    gParticles[index] = p;
    gFreeList[index] = index;
}