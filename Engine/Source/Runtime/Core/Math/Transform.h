#pragma once
#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix4x4.h"

namespace AtomEngine
{
	class Transform
	{
		Vector3 position{ Vector3::ZERO };
		Vector3 scale{ Vector3::UNIT_SCALE };
		Quaternion rotation{ Quaternion::IDENTITY };

		Transform() = default;

		Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
			: position(position), rotation(rotation), scale(scale)
		{
		}

		Matrix4x4 GetMatrix() const
		{
			Matrix4x4 temp;
			temp.MakeAffine(scale, rotation, position);
			return temp;
		}
	};
}


