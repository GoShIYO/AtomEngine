#include "ParticleUtil.hlsli"
#include "ParticleUpdateCommon.hlsli"

StructuredBuffer<ParticleEmitData> gResetData : register(t0);
StructuredBuffer<ParticleMotion> gInputBuffer : register(t1);
RWStructuredBuffer<ParticleVertex> gVertexBuffer : register(u0);
RWStructuredBuffer<ParticleMotion> gOutputBuffer : register(u1);

[RootSignature(Particle_RootSig)]
[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= EmitProperties.MaxParticles)
        return;

    ParticleMotion ParticleState = gInputBuffer[DTid.x];
    ParticleEmitData rd = gResetData[ParticleState.ResetDataIndex];

    ParticleState.Age += deltaTime * rd.AgeRate;
    if (ParticleState.Age >= 1.0)
        return;

    float StepSize = (ParticleState.Position.y > 0.0 && ParticleState.Velocity.y < 0.0) ?
        min(deltaTime, ParticleState.Position.y / -ParticleState.Velocity.y) : deltaTime;

    ParticleState.Position += ParticleState.Velocity * StepSize;
    ParticleState.Velocity += EmitProperties.Gravity * ParticleState.Mass * StepSize;
    
    ParticleState.Color = lerp(rd.StartColor, rd.EndColor, ParticleState.Age);
    ParticleState.Size = lerp(rd.StartSize, rd.EndSize, ParticleState.Age);
    ParticleState.Color *= ParticleState.Age * (1.0 - ParticleState.Age) * (1.0 - ParticleState.Age) * 6.7;
    
    uint index = gOutputBuffer.IncrementCounter();
    if (index >= EmitProperties.MaxParticles)
        return;
    gOutputBuffer[index] = ParticleState;

    ParticleVertex instance;
    instance.position = ParticleState.Position;
    instance.scale = float3(ParticleState.Size, ParticleState.Size, ParticleState.Size);
    instance.color = ParticleState.Color;
    instance.textureId = EmitProperties.TextureID;
    
    gVertexBuffer[gVertexBuffer.IncrementCounter()] = instance;
}