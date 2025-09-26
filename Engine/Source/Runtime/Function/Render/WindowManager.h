#pragma once
#define NOMINMAX

#include <Windows.h>
#include <string>

namespace AtomEngine
{
	struct WindowCreateInfo
	{
		int width = 1920;
		int height = 1080;
		const wchar_t* title = L"AtomEngine";
		bool isFullscreen = false;
	};

	class WindowManager
	{
	public:
		static WindowManager* GetInstance();

	public:
		void* GetWindow();
		void Initialize(WindowCreateInfo info);
		void CloseWindow();
		bool ShouldClose();
		LRESULT WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		const HWND GetWindowHandle() const { return mWindow; }
	private:

		HWND mWindow = nullptr;

		uint32_t mWindowWidth = 0;
		uint32_t mWindowHeight = 0;

		bool mResized = false;
		bool mResizing = false;
		bool mMinimized = false;
		bool mMaximized = false;
		bool mAppPaused = false;

		void OnResize(uint32_t width,uint32_t height);
	};
}
