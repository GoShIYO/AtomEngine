#include "GlobalContext.h"
#include "Runtime/Core/LogSystem/LogSystem.h"
#include "Runtime/Function/Render/WindowManager.h"
#include "Runtime/Function/Input/Input.h"

namespace AtomEngine
{
	GlobalContext gContext;

	void GlobalContext::Initialize()
	{
		StartLog();

		windowManager = WindowManager::GetInstance();
		WindowCreateInfo windowCreateInfo;
		windowManager->Initialize(windowCreateInfo);

		imgui = std::make_unique<ImGuiCommon>();
		imgui->Initialize(windowManager->GetWindowHandle());

		input = Input::GetInstance();
		input->Initialize();
	}

	void GlobalContext::Shutdown()
	{
		imgui->Shutdown();
		input->Shutdown();

		CloseLog();
	}
}

