#pragma once
#include "Editor/ImGuiCommon.h"

namespace AtomEngine
{
	class WindowManager;
	class Input;

	class GlobalContext
	{
	public:
		// 初期化
		void Initialize();
		// 終了
		void Shutdown();
	public:
		/// グローバルコンテキストのコンポーネント
		WindowManager* windowManager = nullptr;
		Input* input = nullptr;
		std::unique_ptr<ImGuiCommon> imgui;
	};

	extern GlobalContext gContext;
}


