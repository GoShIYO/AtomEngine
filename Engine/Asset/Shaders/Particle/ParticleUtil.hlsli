#include "../Random.hlsli"

#define Particle_RootSig \
    "RootFlags(0), " \
    "CBV(b0)," \
    "CBV(b1)," \
    "DescriptorTable(UAV(u0, numDescriptors = 5))," \
    "DescriptorTable(SRV(t0, numDescriptors = 10))," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_BORDER," \
        "addressV = TEXTURE_ADDRESS_BORDER," \
        "addressW = TEXTURE_ADDRESS_BORDER," \
        "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK," \
        "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT)" \

#define MAX_PARTICLE 0x8000

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR;
};

struct Particle
{
    float3 position;
    float3 scale;
    float lifeTime;
    float3 velocity;
    float currentTime;
    float4 color;
};

cbuffer WorldContants : register(b0)
{
    float4x4 ViewProj;
    float4x4 billboardMat;
    float time;
    float deltaTime;
};

class RandomGenerator
{
    float3 seed;
    float3 Generate3D()
    {
        seed = rand3dTo3d(seed);
        return seed;
    }
    float Generate1D()
    {
        seed.x = rand3dTo1d(seed);
        return seed.x;
    }
};