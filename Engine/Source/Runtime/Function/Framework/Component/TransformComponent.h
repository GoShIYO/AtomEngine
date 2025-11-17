#pragma once
#include "Runtime/Core/Math/Transform.h"

namespace AtomEngine
{
	struct TransformComponent : public Transform
	{
        TransformComponent(const Vector3& position_, 
			const Quaternion& rotation_ = Quaternion::IDENTITY,
			const Vector3& scale_ = Vector3::UNIT_SCALE)
			: Transform(position_, rotation_, scale_)
		{
		}
		TransformComponent() = default;
		~TransformComponent() = default;
	};
}
