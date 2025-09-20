#pragma once
#include <chrono>

namespace AtomEngine
{
	class AtomEngine
	{
	public:
		void Initialize();

		void Update();

		void Shutdown();

		float FPS() const { return mFps; }

	protected:
		void LogicalTick(float deltaTime);
		void RenderTick(float deltaTime);

		float CalculateDeltaTime();
		void CalculateFPS(float deltaTime);

		void Tick(float deltaTime);
	protected:
		bool mIsQuit = false;
		std::chrono::steady_clock::time_point mLastTickTime{ std::chrono::steady_clock::now() };

		float mAverageDuration = 0.f;
		int   mFrameCount = 0;
		float mFps = 0;
		static constexpr float kTargetFPS = 1 / 60.0f;
	};
}
