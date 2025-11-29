#include "ParticleUtil.hlsli"

ByteAddressBuffer gFinalInstanceCounter : register(t0);
RWByteAddressBuffer gNumThreadGroups : register(u0);
RWByteAddressBuffer gDrawIndirectArgs : register(u1);

[RootSignature(Particle_RootSig)]
[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint particleCount = gFinalInstanceCounter.Load(0);
    gNumThreadGroups.Store3(0, uint3((particleCount + 63) / 64, 1, 1));
    gDrawIndirectArgs.Store(4, particleCount);
}