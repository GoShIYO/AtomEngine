#include "WindowManager.h"
#include "RenderSystem.h"

#include "Runtime/Core/Utility/Utility.h"
#include "Runtime/Platform/DirectX12/Core/DescriptorHeap.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"
#include "Runtime/Platform/DirectX12/Core/CommandListManager.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include <mfapi.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

#pragma comment(lib, "mfplat.lib")

namespace AtomEngine
{
	static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return static_cast<WindowManager*>(WindowManager::GetInstance())->WindowProcedure(hWnd, msg, wParam, lParam);
	}

	WindowManager* WindowManager::GetInstance()
	{
		static WindowManager instance;
		return &instance;
	}

	void* WindowManager::GetWindow()
	{
		return static_cast<void*>(mHwnd);
	}

	void WindowManager::Initialize(WindowCreateInfo info)
	{
		// COM初期化
		ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
		//Media Foundationの初期化
		ThrowIfFailed(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET));

		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		ImGui_ImplWin32_EnableDpiAwareness();
		float mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

		WNDCLASSEXW wndClass = {};
		wndClass.cbSize = sizeof(wndClass);
		wndClass.style = CS_HREDRAW | CS_VREDRAW;
		wndClass.lpfnWndProc = WndProc;
		wndClass.cbClsExtra = 0;
		wndClass.cbWndExtra = 0;
		wndClass.hInstance = GetModuleHandle(NULL);
		wndClass.hIcon = LoadIcon(0, IDI_APPLICATION);
		wndClass.hCursor = LoadCursor(0, IDC_ARROW);
		wndClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		wndClass.lpszMenuName = 0;
		wndClass.lpszClassName = L"MainWnd";
		wndClass.hIconSm = LoadIcon(0, IDI_APPLICATION);

		RegisterClassExW(&wndClass);

		mWindowWidth = info.width;
		mWindowHeight = info.height;

		RECT rc = { 0, 0, static_cast<LONG>(mWindowWidth), static_cast<LONG>(mWindowHeight) };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
		int width = static_cast<int>(rc.right - rc.left);
		int height = static_cast<int>(rc.bottom - rc.top);

		mHwnd = CreateWindowW(
			wndClass.lpszClassName,
			info.title,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			width, height,
			nullptr,
			nullptr,
			wndClass.hInstance,
			nullptr
		);

		ASSERT(mHwnd != 0);

		DX12Core::InitializeDx12();

		ShowWindow(mHwnd, SW_SHOWDEFAULT);
		// ImGuiの初期化
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

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

			ImGui_ImplWin32_Init(mHwnd);

			ImGui_ImplDX12_InitInfo init_info = {};
			init_info.Device = DX12Core::gDevice.Get();
			init_info.CommandQueue = DX12Core::gCommandManager.GetGraphicsQueue().GetCommandQueue();
			init_info.NumFramesInFlight = 3;
			init_info.RTVFormat = DX12Core::gBackBufferFormat;
			init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;

			init_info.SrvDescriptorHeap = RenderSystem::GetTextureHeap().GetHeapPointer();
			init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
				{
					auto& textureHeap = RenderSystem::GetTextureHeap();
					DescriptorHandle handle = textureHeap.Alloc();

					*out_cpu_handle = D3D12_CPU_DESCRIPTOR_HANDLE(handle);
					*out_gpu_handle = D3D12_GPU_DESCRIPTOR_HANDLE(handle);
				};
			init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) {};
			ImGui_ImplDX12_Init(&init_info);

		}

	}

	void WindowManager::CloseWindow()
	{
		PostQuitMessage(0);
	}

	bool WindowManager::ShouldClose()
	{
		MSG msg;
		bool done = false;
		while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}

		return done;
	}

	LRESULT WindowManager::WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg)
		{
		case WM_SIZE:
			mWindowWidth = (UINT)(UINT64)lParam & 0xFFFF;
			mWindowHeight = (UINT)(UINT64)lParam >> 16;
			OnResize(mWindowWidth, mWindowHeight);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}

		return 0;
	}

	void WindowManager::OnResize(uint32_t width, uint32_t height)
	{
		DX12Core::OnResize(width, height);
	}
}
