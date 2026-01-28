#pragma once
#include "Runtime/Platform/DirectX12/Pipeline/PipelineState.h"

namespace AtomEngine
{
	class Camera;
	class GraphicsContext;
	class ComputeContext;
	class ColorBuffer;
	class DepthBuffer;

	class SSAO
	{
	public:
		static void Initialize();
		static void Shutdown();
		static void Render(GraphicsContext& Context, const float* ProjMat, float NearClipDist, float FarClipDist);
		static void Render(GraphicsContext& Context, const Camera& camera);
		static void LinearizeZ(ComputeContext& Context, const  Camera& camera, uint32_t FrameIndex);
		static bool EnableDebugDraw();
	private:
		static void ComputeAO(ComputeContext& Context, ColorBuffer& Destination, ColorBuffer& DepthBuffer, const float TanHalfFovH);
		static void BlurAndUpsample(ComputeContext& Context,
			ColorBuffer& Destination, ColorBuffer& HiResDepth, ColorBuffer& LoResDepth,
			ColorBuffer* InterleavedAO, ColorBuffer* HighQualityAO, ColorBuffer* HiResAO);
		static void LinearizeZ(ComputeContext& Context, DepthBuffer& Depth, ColorBuffer& LinearDepth, float zMagic);
	};
}

