#pragma once
#include<memory>

namespace AtomEngine
{
	class WindowManager;

	class GlobalContext
	{
	public:
		// 初期化
		void Initialize();
		// 終了
		void Finalize();
	public:
		/// グローバルコンテキストのコンポーネント


	};

	extern GlobalContext gGlobalContext;
}


