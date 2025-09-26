#include "WindowManager.h"
#include "Runtime/Core/Utility/Utility.h"
#include "Runtime/Platform/DirectX12/Core/SwapChain.h"
#include "Runtime/Platform/DirectX12/Core/GraphicsCore.h"
#include "imgui_impl_win32.h"
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
		return static_cast<void*>(mWindow);
	}

	void WindowManager::Initialize(WindowCreateInfo info)
	{
		// COM初期化
		ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
		//Media Foundationの初期化
		ThrowIfFailed(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET));

		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

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

		mWindow = CreateWindowW(
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

		InitializeDx12();

		ShowWindow(mWindow, SW_SHOWDEFAULT);
	}

	void WindowManager::CloseWindow()
	{
		PostQuitMessage(0);
	}

	bool WindowManager::ShouldClose()
	{
		MSG msg;
		BOOL hasMsg;
		while (true)
		{
			hasMsg = PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE);

			if (!hasMsg && !mAppPaused)
				break;

			if (!hasMsg)
				continue;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				return true;
		}

		return false;
	}

	LRESULT WindowManager::WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg)
		{
		case WM_SIZE:
			mWindowWidth = static_cast<uint32_t>(LOWORD(lParam));
			mWindowHeight = static_cast<uint32_t>(HIWORD(lParam));
			mResized = true;

			if (wParam == SIZE_MINIMIZED)
		{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize(mWindowWidth, mWindowHeight);
			}
			else if (wParam == SIZE_RESTORED)
			{
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize(mWindowWidth, mWindowHeight);
				}
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize(mWindowWidth, mWindowHeight);
				}
				else if (mResizing)
				{

		}
				else
				{
					OnResize(mWindowWidth, mWindowHeight);
				}
			}
		return 0;

		case WM_ENTERSIZEMOVE:
			mAppPaused = true;
			mResizing = true;
			return 0;

		case WM_EXITSIZEMOVE:
			mAppPaused = false;
			mResizing = false;
			OnResize(mWindowWidth, mWindowHeight);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	void WindowManager::OnResize(uint32_t width, uint32_t height)
	{
		SwapChain::OnResize(width, height);
	}
}
