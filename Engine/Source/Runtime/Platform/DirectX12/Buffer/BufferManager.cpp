#include "BufferManager.h"
#include "../Core/DirectX12Core.h"

namespace AtomEngine
{
	DepthBuffer gSceneDepthBuffer;
	ColorBuffer gSceneColorBuffer;
	ColorBuffer gSceneNormalBuffer;
	ColorBuffer gPostEffectsBuffer;
	ColorBuffer gOverlayBuffer;

	ShadowBuffer gShadowBuffer;
	ShadowBuffer gRayTracedShadowBuffer;

	ColorBuffer gSSAOFullScreen(Color(1.0f, 1.0f, 1.0f));
	ColorBuffer gLinearDepth[2];

	ColorBuffer gLumaBuffer;
	ColorBuffer gBloomUAV1[2];	// 640x384 (1/3)
	ColorBuffer gBloomUAV2[2];	// 320x192 (1/6) 
	ColorBuffer gBloomUAV3[2];	// 160x96  (1/12)
	ColorBuffer gBloomUAV4[2];	// 80x48   (1/24)
	ColorBuffer gBloomUAV5[2];	// 40x24   (1/48)
	ColorBuffer gLumaLR;

	ByteAddressBuffer gHistogram;

	void InitializeBuffers(uint32_t bufferWidth, uint32_t bufferHeight)
	{
		gSceneColorBuffer.SetClearColor(Color(0x314D79FF));
		gSceneColorBuffer.Create(L"Main Color Buffer", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R10G10B10A2_UNORM);

		gSceneNormalBuffer.Create(L"Normals Buffer", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
		gPostEffectsBuffer.Create(L"Post Effects Buffer", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R32_UINT);

		gSceneDepthBuffer.Create(L"Scene Depth Buffer", bufferWidth, bufferHeight, DXGI_FORMAT_D24_UNORM_S8_UINT);

		gOverlayBuffer.Create(L"UI Overlay", DX12Core::gBackBufferWidth, DX12Core::gBackBufferHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM);

		gShadowBuffer.Create(L"Shadow Map", 2048, 2048);
		gRayTracedShadowBuffer.Create(L"Ray Traced Shadow Map", bufferWidth, bufferHeight);

		gSSAOFullScreen.Create(L"SSAO Full Screen", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R8_UNORM);
		gLinearDepth[0].Create(L"Linear Depth 0", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16_UNORM);
		gLinearDepth[1].Create(L"Linear Depth 1", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16_UNORM);

		gLumaBuffer.Create(L"Luminance", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R8_UNORM);
		uint32_t kBloomWidth = bufferWidth > 2560 ? 1280 : 640;
		uint32_t kBloomHeight = bufferHeight > 1440 ? 768 : 384;
		gLumaLR.Create(L"Luma Buffer", kBloomWidth, kBloomHeight, 1, DXGI_FORMAT_R8_UINT);
		gBloomUAV1[0].Create(L"Bloom Buffer 1a", kBloomWidth, kBloomHeight, 1, DXGI_FORMAT_R10G10B10A2_UNORM);
		gBloomUAV1[1].Create(L"Bloom Buffer 1b", kBloomWidth, kBloomHeight, 1, DXGI_FORMAT_R10G10B10A2_UNORM);
		gBloomUAV2[0].Create(L"Bloom Buffer 2a", kBloomWidth / 2, kBloomHeight / 2, 1, DXGI_FORMAT_R10G10B10A2_UNORM);
		gBloomUAV2[1].Create(L"Bloom Buffer 2b", kBloomWidth / 2, kBloomHeight / 2, 1, DXGI_FORMAT_R10G10B10A2_UNORM);
		gBloomUAV3[0].Create(L"Bloom Buffer 3a", kBloomWidth / 4, kBloomHeight / 4, 1, DXGI_FORMAT_R10G10B10A2_UNORM);
		gBloomUAV3[1].Create(L"Bloom Buffer 3b", kBloomWidth / 4, kBloomHeight / 4, 1, DXGI_FORMAT_R10G10B10A2_UNORM);
		gBloomUAV4[0].Create(L"Bloom Buffer 4a", kBloomWidth / 8, kBloomHeight / 8, 1, DXGI_FORMAT_R10G10B10A2_UNORM);
		gBloomUAV4[1].Create(L"Bloom Buffer 4b", kBloomWidth / 8, kBloomHeight / 8, 1, DXGI_FORMAT_R10G10B10A2_UNORM);
		gBloomUAV5[0].Create(L"Bloom Buffer 5a", kBloomWidth / 16, kBloomHeight / 16, 1, DXGI_FORMAT_R10G10B10A2_UNORM);
		gBloomUAV5[1].Create(L"Bloom Buffer 5b", kBloomWidth / 16, kBloomHeight / 16, 1, DXGI_FORMAT_R10G10B10A2_UNORM);

		gHistogram.Create(L"Histogram", 256, 4);
	}
	void DestroyBuffers()
	{
		gSceneColorBuffer.Destroy();
		gSceneNormalBuffer.Destroy();
		gPostEffectsBuffer.Destroy();
		gOverlayBuffer.Destroy();

		gSceneDepthBuffer.Destroy();

		gShadowBuffer.Destroy();
		gRayTracedShadowBuffer.Destroy();

		gSSAOFullScreen.Destroy();
		gLinearDepth[0].Destroy();
		gLinearDepth[1].Destroy();

		gLumaBuffer.Destroy();
		gLumaLR.Destroy();

		gBloomUAV1[0].Destroy();
		gBloomUAV1[1].Destroy();
		gBloomUAV2[0].Destroy();
		gBloomUAV2[1].Destroy();
		gBloomUAV3[0].Destroy();
		gBloomUAV3[1].Destroy();
		gBloomUAV4[0].Destroy();
		gBloomUAV4[1].Destroy();
		gBloomUAV5[0].Destroy();
		gBloomUAV5[1].Destroy();

		gHistogram.Destroy();
	}
	void OnResize(uint32_t Width, uint32_t Height)
	{
		gOverlayBuffer.Create(L"UI Overlay", DX12Core::gBackBufferWidth, DX12Core::gBackBufferHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM);

	}
}

