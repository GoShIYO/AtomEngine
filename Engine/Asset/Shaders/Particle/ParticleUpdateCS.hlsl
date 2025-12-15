#include "ParticleUtil.hlsli"
#include "ParticleUpdateCommon.hlsli"

StructuredBuffer<ParticleEmitData> gResetData : register(t0);
StructuredBuffer<ParticleMotion> gInputBuffer : register(t1);
RWStructuredBuffer<ParticleVertex> gVertexBuffer : register(u0);
RWStructuredBuffer<ParticleMotion> gOutputBuffer : register(u2);

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

    if (EmitProperties.IsFollow == 1)
    {
        ParticleState.LocalOffset += ParticleState.Velocity * StepSize + EmitProperties.Gravity * ParticleState.Mass * StepSize;
        ParticleState.Position = EmitProperties.EmitPosW + ParticleState.LocalOffset;
    }
    else
    {
        ParticleState.Position += ParticleState.Velocity * StepSize;
        ParticleState.Velocity += EmitProperties.Gravity * ParticleState.Mass * StepSize;
    }
    
    if (rd.Drag > 0.0)
    {
        float dragFactor = pow(rd.Drag, StepSize);
        ParticleState.Velocity *= dragFactor;
    }
    
    ParticleState.Size = lerp(rd.StartSize, rd.EndSize, ParticleState.Age);
    
    float4 interpColor = lerp(rd.StartColor, rd.EndColor, ParticleState.Age);
    float fade = ParticleState.Age * (1.0 - ParticleState.Age) * (1.0 - ParticleState.Age) * 6.7;
    fade = saturate(fade);

    ParticleState.Color.rgb = interpColor.rgb * fade + EmitProperties.EmissiveColor;
    ParticleState.Color.a = interpColor.a;
    StepSize = deltaTime - StepSize;

    uint index = gOutputBuffer.IncrementCounter();
    if (index >= EmitProperties.MaxParticles)
        return;
    gOutputBuffer[index] = ParticleState;

    ParticleVertex instance;
    instance.position = ParticleState.Position;
    instance.scale = float3(ParticleState.Size, ParticleState.Size, ParticleState.Size) * EmitProperties.Scale;
    instance.color = ParticleState.Color * EmitProperties.colorScale;
    instance.textureId = EmitProperties.TextureID;
    
    gVertexBuffer[gVertexBuffer.IncrementCounter()] = instance;
}