#pragma once
#include "Runtime/Platform/DirectX12/Pipline/PiplineState.h"
#include "Runtime/Platform/DirectX12/Core/GraphicsCommon.h"
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "Runtime/Core/Math/MathInclude.h"

namespace AtomEngine
{
	struct MeshVertex
	{
		Vector3 position;
		Vector3 normal;
		Vector2 uv;
	};
	struct MatMaterial
	{
		Vector4 color;
	};
	class PrimitiveMesh;
	class MeshRenderer
	{
	public:
		static void Initialize();

		static void Shutdown();

		static void Render(GraphicsContext& gfxContext, const Camera* camera);

		static void AddObject(const PrimitiveMesh* mesh);

	private:
		static void Sort();
	};
}


