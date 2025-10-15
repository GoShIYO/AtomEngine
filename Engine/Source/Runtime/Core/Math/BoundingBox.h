#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "Transform.h"

#include <numeric>

namespace AtomEngine
{
	class AxisAlignedBox
	{
	public:
		using limit = std::numeric_limits<float>;
		AxisAlignedBox() : mMin(limit::max(), limit::max(), limit::max()), mMax(-limit::max(), -limit::max(), -limit::max()) {}
		AxisAlignedBox(const Vector3& min, const Vector3& max) : mMin(min), mMax(max) {}

		void AddPoint(const Vector3& point)
		{
			mMin = Math::Min(point, mMin);
			mMax = Math::Max(point, mMax);
		}

		void AddBoundingBox(const AxisAlignedBox& box)
		{
			AddPoint(box.mMin);
			AddPoint(box.mMax);
		}

		AxisAlignedBox Union(const AxisAlignedBox& box)
		{
			return AxisAlignedBox(Math::Min(mMin, box.mMin), Math::Max(mMax, box.mMax));
		}

		Vector3 GetMin() const { return mMin; }
		Vector3 GetMax() const { return mMax; }
		Vector3 GetCenter() const { return (mMin + mMax) * 0.5f; }
		Vector3 GetDimensions() const { return Math::Max(mMax - mMin, Vector3::ZERO); }
		float GetRadius() const { return GetDimensions().Length() * 0.5f; }
	private:

		Vector3 mMin;
		Vector3 mMax;
	};

	class OrientedBox
	{
	public:
		OrientedBox() {}

		OrientedBox(const AxisAlignedBox& box)
		{
			m_matrix = (Matrix4x4::MakeScale(box.GetMax() - box.GetMin()));
			m_repr.transition = box.GetMin();
		}

		//friend OrientedBox operator* (const Transform& xform, const OrientedBox& obb)
		//{

		//	return (OrientedBox&)(xform * obb.m_repr);
		//}

		Vector3 GetDimensions() const { return m_matrix.GetX() + m_matrix.GetY() + m_matrix.GetZ(); }
		Vector3 GetCenter() const { return m_repr.transition + GetDimensions() * 0.5f; }

	private:
		Transform m_repr;
		Matrix4x4 m_matrix;
	};

	//inline OrientedBox operator* (const Transform& xform, const AxisAlignedBox& aabb)
	//{
	//	return xform * OrientedBox(aabb);
	//}
}

