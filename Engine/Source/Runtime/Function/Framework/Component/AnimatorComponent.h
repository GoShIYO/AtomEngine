#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include "Runtime/Resource/Animation.h"

namespace AtomEngine
{
	/*struct AnimationState
	{
		enum Mode { kStopped, kPlaying, kLooping };
		Mode mode = kStopped;

		float time = 0.0f;
		float speed = 1.0f;
		float weight = 1.0f;

		void Reset() { time = 0.0f; }
	};
	class AnimatorComponent
	{
	public:
		std::vector<const AnimationClip*> clips;
		std::vector<AnimationState> states;

		bool isPlaying = false;

		void Play(const AnimationClip* clip, bool looped = true)
		{
			clips.clear();
			states.clear();
			clips.push_back(clip);
			states.push_back({ AnimationState::kPlaying, 0.0f, 1.0f, 1.0f });
			isPlaying = true;
		}

		void Update(float deltaTime)
		{
			if (!isPlaying || clips.empty()) return;
			for (size_t i = 0; i < clips.size(); ++i)
			{
				auto& clip = *clips[i];
				auto& state = states[i];
				if (state.mode == AnimationState::kStopped) continue;

				state.time += deltaTime * state.speed;
				if (state.mode == AnimationState::kLooping && state.time > clip.duration)
					state.time = fmod(state.time, clip.duration);
				else if (state.time > clip.duration)
					state.mode = AnimationState::kStopped;
			}
		}
	};*/

}


