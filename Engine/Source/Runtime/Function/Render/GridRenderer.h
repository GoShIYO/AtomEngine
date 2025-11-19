#pragma once
#include "Runtime/Function/Camera/CameraBase.h"
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"
#include "Runtime/Platform/DirectX12/Pipline/PiplineState.h"

namespace AtomEngine
{
	class GridRenderer
	{
	public:
		GridRenderer();
		void Initialize();
		void Shutdown();

		void Render(GraphicsContext& context, const Camera* camera, const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissor);

	private:

		Color xAxisColor;
		Color zAxisColor;
		Color majorLineColor;
		Color minorLineColor;

		RootSignature mGridRootSig;
		GraphicsPSO mGridPSO;
	};
}


