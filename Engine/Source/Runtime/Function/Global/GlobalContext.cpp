#include "GlobalContext.h"
#include "Runtime/Core/LogSystem/LogSystem.h"
#include "Runtime/Function/Render/WindowManager.h"
#include "Runtime/Function/Input/Input.h"

namespace AtomEngine
{
	GlobalContext gContext;

	void GlobalContext::Initialize()
	{
		InitLog();

		windowManager = WindowManager::GetInstance();
		WindowCreateInfo windowCreateInfo;
		windowManager->Initialize(windowCreateInfo);

		input = Input::GetInstance();
		input->Initialize();

	}

	void GlobalContext::Finalize()
	{
		input->Finalize();

		FinalizeLog();
	}
}

