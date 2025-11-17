#pragma once
#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix4x4.h"
#include "BoundingSphere.h"

namespace AtomEngine
{
	struct Transform
	{
		Vector3 transition{ Vector3::ZERO };
		Vector3 scale{ Vector3::UNIT_SCALE };
		Quaternion rotation{ Quaternion::IDENTITY };
		Transform* parent = nullptr;

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
			if (parent)
				return parent->GetMatrix() * temp;
			return temp;
		}

		const Matrix3x3 GetMatrix3x3() const
		{
			Matrix4x4 temp;
			temp.MakeAffine(scale, rotation, transition);
			if (parent)
				return parent->GetMatrix3x3() * Matrix3x3(temp);
			return Matrix3x3(temp);
		}

		void SetScale(float s) { scale *= s; }
		float GetScale() const 
		{ 
			return Math::Max(Math::Max(scale.x, scale.y), scale.z);
		}

		inline Vector3 operator*(const Vector3& vec) const
		{
			return rotation * (vec * GetScale()) + transition;
		}

		inline BoundingSphere operator*(BoundingSphere sphere) const
		{
			return BoundingSphere(*this * sphere.GetCenter(), GetScale() * sphere.GetRadius());
		}

		//MIYAG
		inline Vector3 GetWorldPosition()
		{
			Matrix4x4 tmp = GetMatrix();
			return Vector3{ tmp.mat[3][0],tmp.mat[3][1] ,tmp.mat[3][2] };
		}


	};
}
