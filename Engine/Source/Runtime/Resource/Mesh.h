#pragma once
#include "Runtime/Core/Math/MathInclude.h"

namespace AtomEngine
{

	struct VERTEX
	{
		Vector4 position;
		Vector3 normal;
        Vector2 uv;
	};

	struct Mesh
	{
		uint32_t vbOffset;
		uint32_t vbSize;
		uint32_t vbDepthOffset;
		uint32_t vbDepthSize;
		uint32_t ibOffset;
		uint32_t ibSize;
		uint8_t vbStride;
		uint8_t ibFormat;

	};
}