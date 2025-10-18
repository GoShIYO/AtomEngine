#pragma once
#include "Runtime/Core/Math/MathInclude.h"

namespace AtomEngine
{
	struct Skeleton;
	class SeletonComponent
	{
	public:

		const Skeleton* skeleton = nullptr;

		std::vector<Matrix4x4> skeletonPose;
		std::vector<Matrix4x4> jointMatrix;
	};

}

