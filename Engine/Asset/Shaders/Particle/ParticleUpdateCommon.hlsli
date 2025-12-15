
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
    uint TextureID;
    float3 EmissiveColor;
    uint Emit;
    float3 Scale;
    uint IsFollow;
    uint4 RandIndex[64];
    float colorScale;
};

cbuffer ParticleProperty : register(b1)
{
    EmitterProperty EmitProperties;
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
    float Drag;
    float DragFactor;
};

struct ParticleMotion
{
    float4 Color;
    float3 Position;
    float Mass;
    float3 Velocity;
    float Age;
    float3 LocalOffset;
    float Size;
    float Rotation;
    uint ResetDataIndex;
};