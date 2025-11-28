#pragma once
#include "ParticleCommon.h"
#include "Runtime/Platform/DirectX12/Buffer/GpuBuffer.h"
#include "Runtime/Platform/DirectX12/Pipline/PiplineState.h"
#include "Runtime/Platform/DirectX12/Core/GraphicsCommon.h"
#include "Runtime/Platform/DirectX12/Context/ComputeContext.h"

namespace AtomEngine
{
	struct Particle
	{
		Vector3 position;
		Vector3 scale;
		float lifeTime;
		Vector3 velocity;
		float currentTime;
		Color color;
	};

	struct ParticleVertex
	{
		Vector4 position;
		Vector2 texcoord;
	};
}


