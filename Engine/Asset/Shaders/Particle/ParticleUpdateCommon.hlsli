
struct EmitterProperty
{
    float3 LastEmitPosW;
    float EmitSpeed;
    float3 EmitPosW;
    float FloorHeight;
    float3 EmitDirW;
    float Restitution;
    float3 EmitRightW;
    float EmitterVelocitySensitivity;
    float3 EmitUpW;
    uint MaxParticles;
    float3 Gravity;
    float3 EmissiveColor;
};

cbuffer ParticleProperty : register(b1)
{
    float4 MinStartColor;
    float4 MaxStartColor;
    float4 MinEndColor;
    float4 MaxEndColor;
    EmitterProperty EmitProperties;
    float4 Velocity;
    float4 Size;
    float3 Spread;
    float EmitRate;
    float2 LifeMinMax;
    float2 MassMinMax;
};

struct ParticleEmitData
{
    float AgeRate;
    float RotationSpeed;
    float StartSize;
    float EndSize;
    float3 Velocity;
    float Mass;
    float3 SpreadOffset;
    float Random;
    float4 StartColor;
    float4 EndColor;
};

struct ParticleDesc
{
    float4 Color;
    float3 Position;
    float Mass;
    float3 Velocity;
    float Age;
    float Size;
    float Rotation;
    uint ResetDataIndex;
};