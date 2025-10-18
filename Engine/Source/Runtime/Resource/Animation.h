#pragma once
#include "Runtime/Core/Math/MathInclude.h"

#include <vector>
#include <cstdint>

namespace AtomEngine
{

	struct JointVertex
	{
		uint16_t jointIndices[4];
		float weights[4];
	};

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


}
