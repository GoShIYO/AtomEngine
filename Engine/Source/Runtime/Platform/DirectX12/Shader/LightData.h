#ifndef LIGHT_DATA_H
#define LIGHT_DATA_H
#endif

#ifdef __cplusplus
#include "Runtime/Core/Math/MathInclude.h"
using float3 = AtomEngine::Vector3;
using float4x4 = AtomEngine::Matrix4x4;
using uint = uint32_t;

enum class LightType : uint
{
    Point = 0,      // Point light
    Spot,           // Spot light
    Directional,    // Directional light
};

#endif // 

struct alignas(16) LightData
{
    float3 position;
    float radiusSq;
    float3 color;
    uint type;          // 0=directional, 1=Point, 2=Spot
    float3 direction;
    float innerCos;         // For spot
    float4x4 shadowMatrix;
    float outerCos;
    float3 pad0;
};