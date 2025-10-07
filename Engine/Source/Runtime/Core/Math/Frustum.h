#pragma once
#include "MathInclude.h"
#include "BoundingPlane.h"
#include "BoundingSphere.h"
#include "BoundingBox.h"

namespace AtomEngine
{
	class Frustum
	{
	public:
		Frustum() {}

		Frustum(const Matrix4x4& ProjectionMatrix);

		enum CornerID
		{
			kNearLowerLeft, kNearUpperLeft, kNearLowerRight, kNearUpperRight,
			kFarLowerLeft, kFarUpperLeft, kFarLowerRight, kFarUpperRight
		};

		enum PlaneID
		{
			kNearPlane, kFarPlane, kLeftPlane, kRightPlane, kTopPlane, kBottomPlane
		};

		Vector3         GetFrustumCorner(CornerID id) const { return m_FrustumCorners[id]; }
		BoundingPlane   GetFrustumPlane(PlaneID id) const { return m_FrustumPlanes[id]; }

		bool IntersectSphere(BoundingSphere sphere) const;

		bool IntersectBoundingBox(const AxisAlignedBox& aabb) const;

		friend Frustum operator* (const Transform& xform, const Frustum& frustum);
	private:

		//　透視錐台コンストラクター (ピラミッド型の錐台用)
		void ConstructPerspectiveFrustum(float HTan, float VTan, float NearClip, float FarClip);

		// 正投影錐台コンストラクタ（箱型錐台用）
		void ConstructOrthographicFrustum(float Left, float Right, float Top, float Bottom, float NearClip, float FarClip);

		Vector3 m_FrustumCorners[8];		// 円錐台の角
		BoundingPlane m_FrustumPlanes[6];	// 六面
	};

	inline bool Frustum::IntersectSphere(BoundingSphere sphere) const
	{
		float radius = sphere.GetRadius();
		for (int i = 0; i < 6; ++i)
		{
			if (m_FrustumPlanes[i].DistanceFromPoint(sphere.GetCenter()) + radius < 0.0f)
				return false;
		}
		return true;
	}

	inline bool Frustum::IntersectBoundingBox(const AxisAlignedBox& aabb) const
	{
		for (int i = 0; i < 6; ++i)
		{
			BoundingPlane p = m_FrustumPlanes[i];
			Vector3 farCorner = Math::Select(aabb.GetMin(), aabb.GetMax(), p.GetNormal() > Vector3::ZERO);
			if (p.DistanceFromPoint(farCorner) < 0.0f)
				return false;
		}

		return true;
	}

	inline Frustum operator* (const Transform& xform, const Frustum& frustum)
	{
		Frustum result;

		for (int i = 0; i < 8; ++i)
			result.m_FrustumCorners[i] = xform.rotation * frustum.m_FrustumCorners[i] + xform.transition;

		for (int i = 0; i < 6; ++i)
			result.m_FrustumPlanes[i] = xform * frustum.m_FrustumPlanes[i];

		return result;
	}
}