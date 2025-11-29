#include "ParticleUtil.hlsli"

ByteAddressBuffer gParticleInstance : register(t0);
RWByteAddressBuffer gNumThreadGroups : register(u1);

[RootSignature(Particle_RootSig)]
[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    gNumThreadGroups.Store(0, (gParticleInstance.Load(0) + 63) / 64);

}