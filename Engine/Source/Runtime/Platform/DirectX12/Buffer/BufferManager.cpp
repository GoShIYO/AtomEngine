#include "BufferManager.h"
#include "../Core/SwapChain.h"

namespace AtomEngine
{
	DepthBuffer gSceneDepthBuffer; 
	ColorBuffer gSceneColorBuffer;
	ColorBuffer gSceneNormalBuffer;
	ColorBuffer gPostEffectsBuffer;

	ShadowBuffer gShadowBuffer;


	void InitializeBuffers(uint32_t bufferWidth, uint32_t bufferHeight)
	{
		gSceneColorBuffer.SetClearColor({ 0.192f,0.302f,0.471f,1.0f });
		gSceneColorBuffer.Create(L"Main Color Buffer", bufferWidth, bufferHeight, 1, SwapChain::GetBackBufferFormat());

		gSceneNormalBuffer.Create(L"Normals Buffer", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
		gPostEffectsBuffer.Create(L"Post Effects Buffer", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R32_UINT);

		gSceneDepthBuffer.Create(L"Scene Depth Buffer", bufferWidth, bufferHeight, DXGI_FORMAT_D32_FLOAT);

		gShadowBuffer.Create(L"Shadow Map", 2048, 2048);

	}
	void DestroyBuffers()
	{
        gSceneColorBuffer.Destroy();
        gSceneNormalBuffer.Destroy();
        gPostEffectsBuffer.Destroy();

		gSceneDepthBuffer.Destroy();

		gShadowBuffer.Destroy();

	}
	void OnResize(uint32_t Width, uint32_t Height)
	{

	}
}

