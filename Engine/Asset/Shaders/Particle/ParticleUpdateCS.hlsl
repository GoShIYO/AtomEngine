#include "ParticleUtil.hlsli"
#include "ParticleUpdateCommon.hlsli"

StructuredBuffer<ParticleEmitData> gResetData : register(t0);
StructuredBuffer<ParticleDesc> gInputBuffer : register(t1);
RWStructuredBuffer<ParticleVertex> gVertexBuffer : register(u0);
RWStructuredBuffer<ParticleDesc> gOutputBuffer : register(u1);

[RootSignature(Particle_RootSig)]
[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= EmitProperties.MaxParticles)
        return;

    ParticleDesc ParticleState = gInputBuffer[DTid.x];
    ParticleEmitData rd = gResetData[ParticleState.ResetDataIndex];

    ParticleState.Age += deltaTime * rd.AgeRate;
    if (ParticleState.Age >= 1.0)
        return;

    float StepSize = (ParticleState.Position.y > 0.0 && ParticleState.Velocity.y < 0.0) ?
        min(deltaTime, ParticleState.Position.y / -ParticleState.Velocity.y) : deltaTime;

    ParticleState.Position += ParticleState.Velocity * StepSize;
    ParticleState.Velocity += EmitProperties.Gravity * ParticleState.Mass * StepSize;

    float4 interpColor = lerp(rd.StartColor, rd.EndColor, ParticleState.Age);
    float fade = ParticleState.Age * (1.0 - ParticleState.Age) * (1.0 - ParticleState.Age) * 6.7;
    fade = saturate(fade);

    ParticleState.Color.rgb = interpColor.rgb * fade;
    ParticleState.Color.a = interpColor.a;

    uint index = gOutputBuffer.IncrementCounter();
    if (index >= EmitProperties.MaxParticles)
        return;
    gOutputBuffer[index] = ParticleState;

    ParticleVertex instance;
    instance.position = ParticleState.Position;
    instance.scale = float3(ParticleState.Size, ParticleState.Size, ParticleState.Size);
    instance.color = ParticleState.Color;

    gVertexBuffer[gVertexBuffer.IncrementCounter()] = instance;
}