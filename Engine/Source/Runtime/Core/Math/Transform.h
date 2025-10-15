#pragma once
#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix4x4.h"
#include "BoundingSphere.h"

namespace AtomEngine
{
	class Transform
	{
	public:

		Vector3 transition{ Vector3::ZERO };
		Vector3 scale{ Vector3::UNIT_SCALE };
		Quaternion rotation{ Quaternion::IDENTITY };

		Transform() = default;

		Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
			: transition(position), rotation(rotation), scale(scale)
		{
		}

		Transform(const Vector3& x, const Vector3& y, const Vector3& z, const Vector3& w)
		{
			Matrix4x4 temp(x, y, z, w);
			scale = temp.GetScale();
            rotation = temp.GetRotation();
            transition = temp.GetTrans();
		}

		const Matrix4x4 GetMatrix()const 
		{
			Matrix4x4 temp;
			temp.MakeAffine(scale, rotation, transition);
			return temp;
		}

		const Matrix3x3 GetMatrix3x3() const
		{
			Matrix4x4 temp;
			temp.MakeAffine(scale, rotation, transition);
			return Matrix3x3(temp);
		}

		void SetScale(float s) { scale *= s; }
		float GetScale() const { return scale.x; }

		inline Vector3 operator*(const Vector3& vec) const
		{
			return rotation * (vec * GetScale()) + transition;
		}

		inline BoundingSphere operator*(BoundingSphere sphere) const
		{
			return BoundingSphere(*this * sphere.GetCenter(), GetScale() * sphere.GetRadius());
		}
	};
}
