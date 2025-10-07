#include "EngineCore.h"
#include "Runtime/Core/LogSystem/LogSystem.h"
#include "Runtime/Function/Global/GlobalContext.h"
#include "Runtime/Function/Render/WindowManager.h"
#include "Runtime/Function/Input/Input.h"
#include "Runtime/Function/Render/RenderSystem.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"

#include "Runtime/Application/GameApp.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

namespace AtomEngine
{
	AtomEngine::AtomEngine()
	{
		gContext.Initialize();

		Log("Engine Start");
	}

	void AtomEngine::Run(GameApp& app)
	{
		auto window = gContext.windowManager;

		app.Initialize();

		while (!window->ShouldClose() && !app.Exit())
		{
			const float deltaTime = CalculateDeltaTime();
			Tick(app,deltaTime);
		}
		app.Shutdown();

		mIsQuit = true;
	}

	bool AtomEngine::Shutdown()
	{
		DX12Core::ShutdownDx12();
		Log("Shutdown Dx12\n");

		gContext.Finalize();
		Log("Shutdown Engine\n");
		return mIsQuit;
	}

	void AtomEngine::LogicalTick(float deltaTime)
	{
		gContext.input->Update();
	}

	void AtomEngine::RenderTick(GameApp& app,float deltaTime)
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		app.Render();
		
		RenderSystem::Render(deltaTime);

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
	void AtomEngine::Tick(GameApp& app, float deltaTime)
	{
		LogicalTick(deltaTime);
		CalculateFPS(deltaTime);

		app.Update(deltaTime);

		RenderTick(app,deltaTime);
	}
}
