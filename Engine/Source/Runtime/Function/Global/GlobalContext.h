#pragma once

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
		void Finalize();
	public:
		/// グローバルコンテキストのコンポーネント
		WindowManager* windowManager = nullptr;
		Input* input = nullptr;
	};

	extern GlobalContext gContext;
}


