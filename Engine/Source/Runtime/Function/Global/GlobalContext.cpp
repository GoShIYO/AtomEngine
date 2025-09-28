#include "GlobalContext.h"
#include "Runtime/Core/LogSystem/LogSystem.h"
#include "Runtime/Function/Render/WindowManager.h"
#include "Runtime/Function/Input/Input.h"

namespace AtomEngine
{
	GlobalContext gGlobalContext;

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

		Input* input = Input::GetInstance();
		input->Finalize();


		FinalizeLog();
	}
}

