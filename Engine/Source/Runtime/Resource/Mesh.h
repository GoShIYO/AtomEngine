#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include "Runtime/Core/Math/BoundingBox.h"

namespace AtomEngine
{
	struct Vertex
	{
		Vector3 position;
		Vector2 texcoord;
		Vector3 normal;
		Vector3 tangent;
		Vector3 bitangent;
	};

	struct SubMesh
	{
		uint32_t indexOffset = 0;
		uint32_t indexCount = 0;
		uint32_t vertexOffset = 0;
		uint32_t vertexCount = 0;

		uint32_t materialIndex = 0;
		AxisAlignedBox bounds;
	};

	struct Mesh
	{
		AxisAlignedBox boundingBox;

		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;

		uint32_t stride = 0;

		uint32_t positionOffset = 0;
		uint32_t uvOffset = 0;
		uint32_t normalOffset = 0;
		uint32_t tangentOffset = 0;

		std::vector<SubMesh> subMeshes;

		int32_t skinIndex = -1;
	};
}