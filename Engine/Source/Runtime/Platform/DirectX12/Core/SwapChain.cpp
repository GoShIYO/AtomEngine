#include "SwapChain.h"
#include "GraphicsCore.h"
#include "CommandListManager.h"
#include "Runtime/Function/Render/WindowManager.h"
#include "../Context/GraphicsContext.h"
#include "../Buffer/BufferManager.h"
#include "../Shader/ShaderCompiler.h"

using Microsoft::WRL::ComPtr;

namespace AtomEngine
{
	constexpr uint32_t kSwapChainBufferCount = 3;

	uint32_t gNativeWidth = 0;
	uint32_t gNativeHeight = 0;
	uint32_t gBackBufferWidth = 1920;
	uint32_t gBackBufferHeight = 1080;
	DXGI_FORMAT gBackBufferFormat = DXGI_FORMAT_R10G10B10A2_UNORM;

	ComPtr<IDXGISwapChain4> gSwapChain = nullptr;
	ColorBuffer gFrameBuffers[kSwapChainBufferCount];
	ColorBuffer gPreFrameBuffer;

	UINT gCurrentBuffer = 0;

	eResolution NativeResolution = k1080p;
	eResolution FrameResolution = k1080p;

	RootSignature sPresentRS;
	GraphicsPSO PresentPS(L"Core: PresentSDR");

	void ResolutionToUINT(eResolution res, uint32_t& width, uint32_t& height)
	{
		switch (res)
		{
		default:
		case k720p:
			width = 1280;
			height = 720;
			break;
		case k900p:
			width = 1600;
			height = 900;
			break;
		case k1080p:
			width = 1920;
			height = 1080;
			break;
		case k1440p:
			width = 2560;
			height = 1440;
			break;
		case k1800p:
			width = 3200;
			height = 1800;
			break;
		case k2160p:
			width = 3840;
			height = 2160;
			break;
		}
	}
	void SetNativeResolution(void)
	{
		uint32_t NativeWidth, NativeHeight;

		ResolutionToUINT(eResolution((int)NativeResolution), NativeWidth, NativeHeight);

		if (gNativeWidth == NativeWidth && gNativeHeight == NativeHeight)
			return;

		gNativeWidth = NativeWidth;
		gNativeHeight = NativeHeight;

		gCommandManager.IdleGPU();

		InitializeBuffers(NativeWidth, NativeHeight);
	}
	void SetDisplayResolution(void)
	{
		static int SelectedDisplayRes = FrameResolution;
		if (SelectedDisplayRes == FrameResolution)
			return;

		SelectedDisplayRes = FrameResolution;
		ResolutionToUINT((eResolution)SelectedDisplayRes, gBackBufferWidth, gBackBufferHeight);

		gCommandManager.IdleGPU();

		OnResize(gBackBufferWidth, gBackBufferHeight);

		auto window = WindowManager::GetInstance();

		SetWindowPos(window->GetWindowHandle(), 0, 0, 0, gBackBufferWidth, gBackBufferHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
	void SwapChain::Initialize()
	{
		ASSERT(gSwapChain == nullptr, "Graphics has already been initialized");

		ComPtr<IDXGIFactory4> dxgiFactory;
		ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory)));

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = gBackBufferWidth;
		swapChainDesc.Height = gBackBufferHeight;
		swapChainDesc.Format = gBackBufferFormat;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = kSwapChainBufferCount;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		auto window = WindowManager::GetInstance();

		ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
			gCommandManager.GetCommandQueue(),
			window->GetWindowHandle(),
			&swapChainDesc,
			nullptr,
			nullptr,
			reinterpret_cast<IDXGISwapChain1**>(gSwapChain.GetAddressOf())));

		for (uint32_t i = 0; i < kSwapChainBufferCount; ++i)
		{
			ComPtr<ID3D12Resource> frameResource;
			ThrowIfFailed(gSwapChain->GetBuffer(i, IID_PPV_ARGS(&frameResource)));
			gFrameBuffers[i].CreateFromSwapChain(L"Primary SwapChain Buffer", frameResource.Detach());
		}

		SetNativeResolution();

		gPreFrameBuffer.Create(L"PreDisplay Buffer", gBackBufferWidth, gBackBufferHeight, 1, gBackBufferFormat);

		sPresentRS.Reset(1, 0);
		sPresentRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
		sPresentRS.Finalize(L"Present");

		auto vsBlob = ShaderCompiler::CompileBlob(L"PresentVS.hlsl", L"vs_6_2");
		auto psBlob = ShaderCompiler::CompileBlob(L"PresentPS.hlsl", L"ps_6_2");

		PresentPS.SetRootSignature(sPresentRS);
		PresentPS.SetRasterizerState(RasterizerTwoSided);
		PresentPS.SetBlendState(BlendDisable);
		PresentPS.SetDepthStencilState(DepthStateDisabled);
		PresentPS.SetSampleMask(0xFFFFFFFF);
		PresentPS.SetInputLayout(0, nullptr);
		PresentPS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		PresentPS.SetVertexShader(vsBlob.Get());
		PresentPS.SetPixelShader(psBlob.Get());
		PresentPS.SetRenderTargetFormat(gBackBufferFormat, DXGI_FORMAT_UNKNOWN);
		PresentPS.Finalize();
	}

	void SwapChain::Finalize()
	{
		gSwapChain->SetFullscreenState(FALSE, nullptr);
		gSwapChain.Reset();

		for (UINT i = 0; i < kSwapChainBufferCount; ++i)
			gFrameBuffers[i].Destroy();

		gPreFrameBuffer.Destroy();
	}

	void SwapChain::OnResize(uint32_t width, uint32_t height)
	{
		gCommandManager.IdleGPU();

		gBackBufferWidth = width;
		gBackBufferHeight = height;

		gPreFrameBuffer.Create(L"PreDisplay Buffer", width, height, 1, gBackBufferFormat);

		for (uint32_t i = 0; i < kSwapChainBufferCount; ++i)
			gFrameBuffers[i].Destroy();

		ASSERT(gSwapChain != nullptr);
		ThrowIfFailed(gSwapChain->ResizeBuffers(kSwapChainBufferCount, width, height, gBackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING));

		for (uint32_t i = 0; i < kSwapChainBufferCount; ++i)
		{
			ComPtr<ID3D12Resource> frameBuffer;
			ThrowIfFailed(gSwapChain->GetBuffer(i, IID_PPV_ARGS(&frameBuffer)));
			gFrameBuffers[i].CreateFromSwapChain(L"Primary SwapChain Buffer", frameBuffer.Detach());
		}

		gCurrentBuffer = 0;

		gCommandManager.IdleGPU();
	}

	void SwapChain::Present()
	{
		GraphicsContext& Context = GraphicsContext::Begin(L"Present");

		Context.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		Context.ClearColor(gSceneColorBuffer);

		Context.SetRootSignature(sPresentRS);
		Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// バッファを読み取ってスワップチェーンバッファに書き込み
		Context.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		Context.SetDynamicDescriptor(0, 0, gSceneColorBuffer.GetSRV());
		Context.SetPipelineState(PresentPS);
		Context.TransitionResource(gFrameBuffers[gCurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
		Context.SetRenderTarget(gFrameBuffers[gCurrentBuffer].GetRTV());
		Context.SetViewportAndScissor(0, 0, gNativeWidth, gNativeHeight);
		Context.ClearColor(gFrameBuffers[gCurrentBuffer]);

		Context.Draw(3);
		Context.TransitionResource(gFrameBuffers[gCurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);
		Context.Finish();

		gSwapChain->Present(1, 0);

		gCurrentBuffer = gSwapChain->GetCurrentBackBufferIndex();

		SetNativeResolution();
		SetDisplayResolution();
	}

	uint32_t SwapChain::GetBackBufferWidth()
	{
		return gBackBufferWidth;
	}
	uint32_t SwapChain::GetBackBufferHeight()
	{
		return gBackBufferHeight;
	}
	DXGI_FORMAT SwapChain::GetBackBufferFormat()
	{
		return gBackBufferFormat;
	}
}

