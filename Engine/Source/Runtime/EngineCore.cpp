#include "EngineCore.h"
#include "Runtime/Core/Log/LogSystem.h"
#include "Runtime/Function/Global/GlobalContext.h"
#include "Runtime/Function/Render/WindowManager.h"

namespace AtomEngine
{
	void AtomEngine::Initialize()
	{
		g_GlobalContext.Initialize();

		Log("Engine Start");
	}

	void AtomEngine::Update()
	{
		auto window = WindowManager::GetInstance();
		while (window->ShouldClose())
		{
			const float deltaTime = CalculateDeltaTime();
			Tick(deltaTime);
		}
	}

	void AtomEngine::Shutdown()
	{
		Log("Engine Shutdown");

		g_GlobalContext.Finalize();
	}

	void AtomEngine::LogicalTick(float deltaTime)
	{

	}

	void AtomEngine::RenderTick(float deltaTime)
	{

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

	
		//g_GlobalContext.m_render_system->swapLogicRenderData();

		RenderTick(deltaTime);
	}
}

