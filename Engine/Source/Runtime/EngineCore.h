#pragma once
#include <chrono>

class GameApp;

namespace AtomEngine
{
	class AtomEngine
	{
	public:
		AtomEngine();
		virtual ~AtomEngine() = default;

		void Run(GameApp& app);
		bool Shutdown();

		float FPS() const { return mFps; }

	protected:
		void LogicalTick(float deltaTime);
		void RenderTick(GameApp& app,float deltaTime);
		void Tick(GameApp& app,float deltaTime);

	private:
		float CalculateDeltaTime();
		void CalculateFPS(float deltaTime);

	protected:
		bool mIsQuit = false;
		std::chrono::steady_clock::time_point mLastTickTime{ std::chrono::steady_clock::now() };

		float mAverageDuration = 0.f;
		int   mFrameCount = 0;
		float mFps = 0;
		static constexpr float kTargetFPS = 1.f / 60.f;
	};
}
