#include "BufferManager.h"
#include "../Core/DirectX12Core.h"

namespace AtomEngine
{
	DepthBuffer gSceneDepthBuffer; 
	ColorBuffer gSceneColorBuffer;
	ColorBuffer gSceneNormalBuffer;
	ColorBuffer gPostEffectsBuffer;

	ShadowBuffer gShadowBuffer;

	ColorBuffer gSSAOFullScreen(Color(1.0f,1.0f,1.0f));
	ColorBuffer gLinearDepth[2];

	void InitializeBuffers(uint32_t bufferWidth, uint32_t bufferHeight)
	{
		gSceneColorBuffer.SetClearColor({ 0.192f,0.302f,0.471f,1.0f });
		gSceneColorBuffer.Create(L"Main Color Buffer", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R11G11B10_FLOAT);

		gSceneNormalBuffer.Create(L"Normals Buffer", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
		gPostEffectsBuffer.Create(L"Post Effects Buffer", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R32_UINT);

		gSceneDepthBuffer.Create(L"Scene Depth Buffer", bufferWidth, bufferHeight, DXGI_FORMAT_D24_UNORM_S8_UINT);

		gShadowBuffer.Create(L"Shadow Map", 2048, 2048);

		gSSAOFullScreen.Create(L"SSAO Full Screen", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R8_UNORM);
		gLinearDepth[0].Create(L"Linear Depth 0", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16_UNORM);
		gLinearDepth[1].Create(L"Linear Depth 1", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16_UNORM);
	}
	void DestroyBuffers()
	{
        gSceneColorBuffer.Destroy();
        gSceneNormalBuffer.Destroy();
        gPostEffectsBuffer.Destroy();

		gSceneDepthBuffer.Destroy();

		gShadowBuffer.Destroy();

		gSSAOFullScreen.Destroy();
		gLinearDepth[0].Destroy();
		gLinearDepth[1].Destroy();
	}
	void OnResize(uint32_t Width, uint32_t Height)
	{

	}
}

