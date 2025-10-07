#pragma once
#include "Runtime/Core/Math/Transform.h"

namespace AtomEngine
{
	class TransformComponent
	{
	public:
		~TransformComponent() = default;

	private:
		Transform mTransform;

	};
}


