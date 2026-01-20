#pragma once
#include "Runtime/Core/Math/MathInclude.h"

#include <vector>
#include <cstdint>

namespace AtomEngine
{
    template<typename T>
    struct Keyframe
    {
        float time;
        T value;
    };

    using KeyframeVec3 = Keyframe<Vector3>;
    using KeyframeQuat = Keyframe<Quaternion>;

	struct AnimationCurve
	{
		std::vector<KeyframeVec3> translation;
		std::vector<KeyframeQuat> rotation;
		std::vector<KeyframeVec3> scale;
		uint32_t targetJoint;
	};

	struct AnimationClip
	{
		std::string name;
		float duration;
		std::vector<AnimationCurve> curves;
	};

	struct AnimationState
	{
		enum eMode { kStopped, kPlaying, kLooping };
		eMode state;
		float time;
		AnimationState() : state(kStopped), time(0.0f) {}
	};

	inline Vector3 CalculateValue(const std::vector<KeyframeVec3>& keyframes, float time)
	{
		assert(!keyframes.empty());
		if (keyframes.size() == 1 || time <= keyframes[0].time)
		{
			return keyframes[0].value;
		}

		for (size_t index = 0; index < keyframes.size() - 1; ++index)
		{
			size_t nextIndex = index + 1;
			if (keyframes[index].time <= time && time <= keyframes[nextIndex].time)
			{
				float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
				return Math::Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
			}
		}

		return (*keyframes.rbegin()).value;
	}

	inline Quaternion CalculateRotation(const std::vector<KeyframeQuat>& keyframes, float time)
	{
		assert(!keyframes.empty());
		if (keyframes.size() == 1 || time <= keyframes[0].time)
		{
			return keyframes[0].value;
		}

		for (size_t index = 0; index < keyframes.size() - 1; ++index)
		{
			size_t nextIndex = index + 1;
			if (keyframes[index].time <= time && time <= keyframes[nextIndex].time)
			{
				float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
				return Slerp(keyframes[index].value, keyframes[nextIndex].value, t, true);
			}
		}

		return (*keyframes.rbegin()).value;
	}
	inline Vector3 CalculateValueLoop(const std::vector<KeyframeVec3>& keyframes, float time, float duration)
	{
		assert(!keyframes.empty());
		if (keyframes.size() == 1)
			return keyframes[0].value;

		const KeyframeVec3& first = keyframes.front();
		const KeyframeVec3& last = keyframes.back();

		if (time <= last.time)
		{
			for (size_t index = 0; index < keyframes.size() - 1; ++index)
			{
				size_t nextIndex = index + 1;
				if (keyframes[index].time <= time && time <= keyframes[nextIndex].time)
				{
					float t = (time - keyframes[index].time) /
						(keyframes[nextIndex].time - keyframes[index].time);
					return Math::Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
				}
			}
			return last.value;
		}

		float t = (time - last.time) / (duration - last.time + first.time);
		return Math::Lerp(last.value, first.value, t);
	}
	inline Quaternion CalculateRotationLoop(const std::vector<KeyframeQuat>& keyframes, float time, float duration)
	{
		assert(!keyframes.empty());
		if (keyframes.size() == 1)
			return keyframes[0].value;

		const KeyframeQuat& first = keyframes.front();
		const KeyframeQuat& last = keyframes.back();

		if (time <= last.time)
		{
			for (size_t index = 0; index < keyframes.size() - 1; ++index)
			{
				size_t nextIndex = index + 1;
				if (keyframes[index].time <= time && time <= keyframes[nextIndex].time)
				{
					float t = (time - keyframes[index].time) /
						(keyframes[nextIndex].time - keyframes[index].time);
					return Slerp(keyframes[index].value, keyframes[nextIndex].value, t,true);
				}
			}
			return last.value;
		}

		float t = (time - last.time) / (duration - last.time + first.time);
		return Slerp(last.value, first.value, t, true);
	}

}
