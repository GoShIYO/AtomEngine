#include "ParticleUtil.hlsli"
#include "ParticleUpdateCommon.hlsli"

StructuredBuffer<ParticleEmitData> gResetData : register(t0);
RWStructuredBuffer<ParticleDesc> gOutputBuffer : register(u1);

[RootSignature(Particle_RootSig)]
[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint index = gOutputBuffer.IncrementCounter();
    if (index >= EmitProperties.MaxParticles)
        return;
    
    RandomGenerator rng;
    rng.seed = float3(index + time * 11.37, DTid.x + time * 21.43, index + DTid.x + time * 4.71);

    uint resetTableSize = EmitProperties.MaxParticles > 0 ? EmitProperties.MaxParticles : 1;
    uint ResetDataIndex = uint(rng.Generate1D() * (float)resetTableSize);
    
        if (ResetDataIndex >= resetTableSize) ResetDataIndex = resetTableSize - 1;

    ParticleEmitData rd = gResetData[ResetDataIndex];

    float3 emitterVelocity = EmitProperties.EmitPosW - EmitProperties.LastEmitPosW;
    float3 randDir = rd.Velocity.x * EmitProperties.EmitRightW + rd.Velocity.y * EmitProperties.EmitUpW + rd.Velocity.z * EmitProperties.EmitDirW;
    float3 newVelocity = emitterVelocity * EmitProperties.EmitterVelocitySensitivity + randDir;
    float3 adjustedPosition = EmitProperties.EmitPosW - emitterVelocity * rng.Generate3D().x + rng.Generate3D().y;

    //ÐÂ¤·¤¤¥Ñ©`¥Æ¥£¥¯¥ë¤òÉú³É
    ParticleDesc newParticle = (ParticleDesc)0;
    newParticle.Position = adjustedPosition;
    newParticle.Rotation = 0.0;
    newParticle.Velocity = newVelocity + EmitProperties.EmitDirW * EmitProperties.EmitSpeed;
    newParticle.Mass = rd.Mass;
    newParticle.Age = 0.0;
    newParticle.ResetDataIndex = ResetDataIndex;
    newParticle.Color = rd.StartColor;
    newParticle.Size = rd.StartSize;
    gOutputBuffer[index] = newParticle;
}