#pragma once
#include "Runtime/Core/Math/MathInclude.h"

namespace AtomEngine
{
	struct PrewView
	{
		Matrix4x4 ViewProj;
		Matrix4x4 billboardMat;
		float time;
		float deltaTime;
	};

	_declspec(align(16))struct EmitterConstants
	{
		Vector3 translate;
		float radius;
		uint32_t count;
		float frequency;
		float frequencyTime;
		uint32_t emit;
	};
	constexpr uint32_t kMaxParticles = 0x4000;
}