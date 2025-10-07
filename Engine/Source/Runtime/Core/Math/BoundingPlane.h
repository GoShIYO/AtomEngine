#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"

namespace AtomEngine
{
    class BoundingPlane
    {
    public:

        BoundingPlane() {}
        BoundingPlane(const Vector3& normalToPlane, float distanceFromOrigin) : m_repr(normalToPlane, distanceFromOrigin) {}
        BoundingPlane(const Vector3& pointOnPlane, const Vector3& normalToPlane);
        BoundingPlane(float A, float B, float C, float D) : m_repr(A, B, C, D) {}
        BoundingPlane(const BoundingPlane& plane) : m_repr(plane.m_repr) {}
        explicit BoundingPlane(const Vector4& plane) : m_repr(plane) {}

        inline operator Vector4() const { return m_repr; }

        // 飛行機が向いている方向を返します。(正規化されない可能性があります)
        Vector3 GetNormal(void) const { return Vector3(m_repr); }

        // 平面上の最近接点を返します
        Vector3 GetPointOnPlane(void) const { return -GetNormal() * m_repr.w; }

        // 3Dポイントからの距離
        float DistanceFromPoint(const Vector3& point) const
        {
            return Math::Dot(point, GetNormal()) + m_repr.w;
        }

        // 同次点からの距離
        float DistanceFromPoint(const Vector4& point) const
        {
            return Math::Dot(point, m_repr);
        }

        friend BoundingPlane operator* (const Transform& xform, BoundingPlane plane)
        {
            Vector3 normalToPlane = xform.rotation * plane.GetNormal();
            float distanceFromOrigin = plane.m_repr.w - Math::Dot(normalToPlane, xform.transition);
            return BoundingPlane(normalToPlane, distanceFromOrigin);
        }

        // 平面を変換する最も効率的な方法。(1つの四元数ベクトル回転と 1 つのドット積が含まれます。)
        friend BoundingPlane operator* (const Matrix4x4& xform, BoundingPlane plane)
        {
            Vector3 normalToPlane = xform.GetRotation() * plane.GetNormal();
            float distanceFromOrigin = plane.m_repr.w - Math::Dot(normalToPlane, xform.GetTrans());
            return BoundingPlane(normalToPlane, distanceFromOrigin);
        }

    private:

        Vector4 m_repr;
    };


    inline BoundingPlane::BoundingPlane(const Vector3& pointOnPlane, const Vector3& normalToPlane)
    {
        auto nrmPlane = Math::Normalize(normalToPlane);
        m_repr = Vector4(normalToPlane, -Math::Dot(pointOnPlane, nrmPlane));
    }

    inline BoundingPlane PlaneFromPointsCCW(const Vector3& A, const Vector3& B, const Vector3& C)
    {
        return BoundingPlane(A, Math::Cross(B - A, C - A));
    }

}


