#include "ImGuiCommon.h"
#include "Runtime/Function/Render/RenderSystem.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"
#include "Runtime/Platform/DirectX12/Core/CommandListManager.h"

namespace AtomEngine
{
	void ImGuiCommon::Initialize(HWND hwnd)
	{
		ImGui_ImplWin32_EnableDpiAwareness();
		float mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
		io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;

		// セットImGuiスタイル
		ImGui::StyleColorsDark();

		//スケーリングの設定
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(mainScale);
		style.FontScaleDpi = mainScale;
		io.ConfigDpiScaleFonts = true;
		io.ConfigDpiScaleViewports = true;

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplWin32_Init(hwnd);

		ImGui_ImplDX12_InitInfo init_info = {};
		init_info.Device = DX12Core::gDevice.Get();
		init_info.CommandQueue = DX12Core::gCommandManager.GetGraphicsQueue().GetCommandQueue();
		init_info.NumFramesInFlight = 3;
		init_info.RTVFormat = DX12Core::gBackBufferFormat;
		init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;

		init_info.SrvDescriptorHeap = Renderer::GetTextureHeap().GetHeapPointer();
		init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
			{
				auto& textureHeap = Renderer::GetTextureHeap();
				DescriptorHandle handle = textureHeap.Alloc();

				*out_cpu_handle = D3D12_CPU_DESCRIPTOR_HANDLE(handle);
				*out_gpu_handle = D3D12_GPU_DESCRIPTOR_HANDLE(handle);
			};
		init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) {};
		ImGui_ImplDX12_Init(&init_info);
	}

	void ImGuiCommon::Bengin()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiCommon::Render(GraphicsContext& Context)
	{
		ImGui::Render();
		auto& textureHeap = Renderer::GetTextureHeap();
		Context.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, textureHeap.GetHeapPointer());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Context.GetCommandList());
	}

	void ImGuiCommon::ShowPerformanceWindow(float deltaTime)
	{
		static float frameRates[1000] = {};
		static int frameIndex = 0;
		const float frameRate = ImGui::GetIO().Framerate;
		frameRates[frameIndex] = ImGui::GetIO().Framerate;
		frameIndex = (frameIndex + 1) % IM_ARRAYSIZE(frameRates);

		ImGui::Begin("Performance");
		ImGui::Text("FPS: %.2f", frameRate);
		ImGui::Text("DeltaTime: %f", deltaTime);
		ImGui::Text("Frame Time: %.3f ms", 1000.0f / frameRate);
		ImGui::PlotLines("Frame Rate", frameRates, IM_ARRAYSIZE(frameRates), 0, nullptr, 0.0f, 144.0f, ImVec2(0, 160));
		ImGui::End();
	}

	void ImGuiCommon::Shutdown()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
}