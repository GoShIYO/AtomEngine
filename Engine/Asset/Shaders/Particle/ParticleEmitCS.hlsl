#include "ParticleUtil.hlsli"
#include "ParticleUpdateCommon.hlsli"

StructuredBuffer<ParticleEmitData> gResetData : register(t0);
RWStructuredBuffer<ParticleMotion> gOutputBuffer : register(u2);

[RootSignature(Particle_RootSig)]
[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint index = gOutputBuffer.IncrementCounter();
    if (index >= EmitProperties.MaxParticles || EmitProperties.Emit == 0)
        return;
    
    uint ResetDataIndex = EmitProperties.RandIndex[DTid.x].x;
    ParticleEmitData rd = gResetData[ResetDataIndex];

    float3 emitterVelocity = EmitProperties.EmitPosW - EmitProperties.LastEmitPosW;
    float3 randDir = rd.Velocity.x * EmitProperties.EmitRightW + rd.Velocity.y * EmitProperties.EmitUpW + rd.Velocity.z * EmitProperties.EmitDirW;
    float3 newVelocity = emitterVelocity * EmitProperties.EmitterVelocitySensitivity + randDir;
    float3 adjustedPosition = EmitProperties.EmitPosW - emitterVelocity * rd.Random + rd.SpreadOffset;
    
    //ÐÂ¤·¤¤¥Ñ©`¥Æ¥£¥¯¥ë¤òÉú³É
    ParticleMotion newParticle;
    newParticle.Position = adjustedPosition;
    newParticle.LocalOffset = adjustedPosition - EmitProperties.EmitPosW;
    newParticle.Rotation = 0.0;
    newParticle.Velocity = newVelocity + EmitProperties.EmitDirW * EmitProperties.EmitSpeed;
    newParticle.Mass = rd.Mass;
    newParticle.Age = 0.0;
    newParticle.ResetDataIndex = ResetDataIndex;
    newParticle.Color = rd.StartColor;
    newParticle.Size = rd.StartSize;
    gOutputBuffer[index] = newParticle;
}