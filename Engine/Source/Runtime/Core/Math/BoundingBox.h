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
		void Transform(const Matrix4x4& mat)
		{
			Vector3 corners[8] = {
				{mMin.x, mMin.y, mMin.z},
				{mMax.x, mMin.y, mMin.z},
				{mMin.x, mMax.y, mMin.z},
				{mMax.x, mMax.y, mMin.z},
				{mMin.x, mMin.y, mMax.z},
				{mMax.x, mMin.y, mMax.z},
				{mMin.x, mMax.y, mMax.z},
				{mMax.x, mMax.y, mMax.z},
			};

			AxisAlignedBox newBox;
			for (auto& c : corners)
				newBox.AddPoint(Math::TransformCoord(c, mat));
			*this = newBox;
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

		OrientedBox(const Matrix3x3& basis, const Vector3& translation) : 
			m_basis(basis), m_translation(translation) {}
		OrientedBox(const AxisAlignedBox& box)
		{
			m_basis = (Matrix3x3::MakeScale(box.GetMax() - box.GetMin()));
			m_translation = box.GetMin();
		}

		friend OrientedBox operator* (const Transform& xform, const OrientedBox& obb)
		{
			Matrix3x3 m = xform.GetMatrix3x3();
			Matrix3x3 mat3 = m * obb.m_basis;
			Vector3 translation = Math::TransformNormal(obb.m_translation, Matrix4x4(m)) + xform.transition;
			return OrientedBox(mat3, translation);
		}

		Vector3 GetDimensions() const { return m_basis.GetX() + m_basis.GetY() + m_basis.GetZ(); }
		Vector3 GetCenter() const { return m_translation + GetDimensions() * 0.5f; }

	private:
		Matrix3x3 m_basis;
		Vector3 m_translation;
	};

	inline OrientedBox operator* (const Transform& xform, const AxisAlignedBox& aabb)
	{
		return xform * OrientedBox(aabb);
	}
}

