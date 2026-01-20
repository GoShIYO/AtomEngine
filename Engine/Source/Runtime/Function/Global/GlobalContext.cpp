#include "GlobalContext.h"
#include "Runtime/Core/LogSystem/LogSystem.h"
#include "Runtime/Function/Render/WindowManager.h"
#include "Runtime/Function/Input/Input.h"
#include "Runtime/Function/Audio/Audio.h"
#include "Runtime/Core/Math/Random.h"
#include<time.h>

namespace AtomEngine
{
	GlobalContext gContext;

	void GlobalContext::Initialize()
	{
		StartLog();

		windowManager = WindowManager::GetInstance();
		WindowCreateInfo windowCreateInfo;
		windowCreateInfo.title = L"AtomEngine";
		windowManager->Initialize(windowCreateInfo);

		imgui = std::make_unique<ImGuiCommon>();
		imgui->Initialize(windowManager->GetWindowHandle());

		input = Input::GetInstance();
		input->Initialize();

		audio = Audio::GetInstance();
		audio->Initialize();

		Random::Initialize();
	}

	void GlobalContext::Shutdown()
	{
		imgui->Shutdown();
		input->Shutdown();

		CloseLog();
	}
}

