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

		Matrix4x4 matrix{ Matrix4x4::IDENTITY };

		Transform() = default;

		Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
			: transition(position), rotation(rotation), scale(scale)
		{
		}
		Transform(const Matrix4x4& m, const Vector3& trans = Vector3::ZERO)
		{
			matrix = m;
			transition = trans;
		}

		Transform(const Vector3& x, const Vector3& y, const Vector3& z, const Vector3& w)
		{
			matrix.MakeAffine(x, y, z);
			transition = w;
		}

		Matrix4x4 GetMatrix()
		{
			matrix.MakeAffine(scale, rotation, transition);
			return matrix;
		}

		void SetScale(float s) { scale *= s; }
		float GetScale() const { return scale.x; }

		//inline Vector3 operator*(const Vector3& v)const { return matrix * v + transition; }
		inline Transform operator*(const Transform& t)const
		{
			return Transform(matrix * t.matrix, *this * t.transition);
		}

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
