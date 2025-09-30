#include "EngineCore.h"
#include "Runtime/Core/LogSystem/LogSystem.h"
#include "Runtime/Function/Global/GlobalContext.h"
#include "Runtime/Function/Render/WindowManager.h"
#include "Runtime/Function/Input/Input.h"
#include "Runtime/Platform/DirectX12/Core/RenderCore.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

namespace AtomEngine
{
	AtomEngine::AtomEngine()
	{
		gGlobalContext.Initialize();

		Log("Engine Start");
	}

	void AtomEngine::Update()
	{
		auto window = WindowManager::GetInstance();
		while (!window->ShouldClose())
		{
			const float deltaTime = CalculateDeltaTime();
			Tick(deltaTime);
		}
		mIsQuit = true;
	}

	bool AtomEngine::Shutdown()
	{
		DX12Core::ShutdownDx12();
		Log("Shutdown Dx12\n");

		gGlobalContext.Finalize();
		Log("Shutdown Engine\n");
		return mIsQuit;
	}

	void AtomEngine::LogicalTick(float deltaTime)
	{
		Input::GetInstance()->Update();
	}

	void AtomEngine::RenderTick(float deltaTime)
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();


		RenderCore::Render(deltaTime);


		DX12Core::Present();
	}

	float AtomEngine::CalculateDeltaTime()
	{
		float deltaTime;
		{
			using namespace std::chrono;

			steady_clock::time_point tickTimePoint = steady_clock::now();
			duration<float> time_span = tickTimePoint - mLastTickTime;
			deltaTime = time_span.count();

			mLastTickTime = tickTimePoint;
		}
		return deltaTime;
	}
	void AtomEngine::CalculateFPS(float deltaTime)
	{
		mFrameCount++;

		if (mFrameCount == 1)
		{
			mAverageDuration = deltaTime;
		}
		else
		{
			mAverageDuration = mAverageDuration * (1.0f - kTargetFPS) + deltaTime * kTargetFPS;
		}

		mFps = 1.0f / mAverageDuration;
	}
	void AtomEngine::Tick(float deltaTime)
	{
		LogicalTick(deltaTime);
		CalculateFPS(deltaTime);


		RenderTick(deltaTime);
	}
}

