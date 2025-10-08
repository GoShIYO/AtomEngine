#pragma once
#include<d3d12shader.h>

#ifdef __cplusplus
#include "Runtime/Core/Math/MathInclude.h"
using namespace AtomEngine;
using float4 = Vector4;
using float3 = Vector3;
using float2 = Vector2;
using float4x4 = Matrix4x4;
using float3x3 = Matrix3x3;
#endif

struct alignas(16) GlobalConstants
{
	float4x4 ViewProjMatrix;
	float4x4 SunShadowMatrix;
	float3 CameraPos;
	float pad0;
	float3 SunDirection;
	float pad1;
	float3 SunIntensity;
	float pad2;
	float IBLRange;
	float IBLBias;
	float2 pad3;
};
