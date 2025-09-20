#include "GlobalContext.h"
#include "Runtime/Core/Log/LogSystem.h"
#include "Runtime/Function/Render/WindowManager.h"
#include "Runtime/Function/Input/Input.h"

namespace AtomEngine
{
	GlobalContext g_GlobalContext;

	void GlobalContext::Initialize()
	{
		InitLog();

		
		WindowManager* windowManager = WindowManager::GetInstance();
		WindowCreateInfo windowCreateInfo;
		windowManager->Initialize(windowCreateInfo);

		Input* input = Input::GetInstance();
		input->Initialize();
	}

	void GlobalContext::Finalize()
	{


		FinalizeLog();
	}
}

