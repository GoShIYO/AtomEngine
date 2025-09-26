#pragma once
#include "Runtime/Platform/DirectX12/Context/CommandContext.h"
#include "Runtime/Platform/DirectX12/Buffer/ColorBuffer.h"
#include "Runtime/Platform/DirectX12/Buffer/DepthBuffer.h"
#include "Runtime/Platform/DirectX12/Core/SwapChain.h"

namespace AtomEngine
{ 
	class RenderPassBase
	{
	public:

		virtual void RenderScene(CommandContext& gfxContext) = 0;
		virtual void RenderScene(CommandContext& gfxContext, ...) {};

	public:

		static void Initialize(uint32_t bufferWidth,uint32_t bufferHeight);
		static void Finalize();

	public:
		static inline ColorBuffer gSceneColorBuffer;
		static inline DepthBuffer gSceneDepthBuffer;
	};

	inline void RenderPassBase::Initialize(uint32_t bufferWidth, uint32_t bufferHeight)
	{
		gSceneColorBuffer.Create(L"Main Color Buffer", bufferWidth, bufferHeight, 1, SwapChain::GetBackBufferFormat());
        gSceneDepthBuffer.Create(L"Main Depth Buffer", bufferWidth, bufferHeight, 1, DXGI_FORMAT_D32_FLOAT);
	}

    inline void RenderPassBase::Finalize()
    {
        gSceneColorBuffer.Destroy();
        gSceneDepthBuffer.Destroy();
    }
};



