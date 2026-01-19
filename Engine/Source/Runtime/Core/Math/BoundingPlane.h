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
        BoundingPlane(const Vector3& normalToPlane, float distanceFromOrigin) : mRepr(normalToPlane, distanceFromOrigin) {}
        BoundingPlane(const Vector3& pointOnPlane, const Vector3& normalToPlane);
        BoundingPlane(float A, float B, float C, float D) : mRepr(A, B, C, D) {}
        BoundingPlane(const BoundingPlane& plane) : mRepr(plane.mRepr) {}
        explicit BoundingPlane(const Vector4& plane) : mRepr(plane) {}

        inline operator Vector4() const { return mRepr; }

        // 平面が向いている方向を返します。(正規化されない可能性があります)
        Vector3 GetNormal(void) const { return Vector3(mRepr).NormalizedCopy(); }

        // 平面上の最近接点を返します
        Vector3 GetPointOnPlane(void) const { return -GetNormal() * mRepr.w; }

        // 3Dポイントからの距離
        float DistanceFromPoint(const Vector3& point) const
        {
            return Math::Dot(point, GetNormal()) + mRepr.w;
        }

        // 同次点からの距離
        float DistanceFromPoint(const Vector4& point) const
        {
            return Math::Dot(point, mRepr);
        }

        friend BoundingPlane operator* (const Transform& xform, BoundingPlane plane)
        {
            Vector3 normalToPlane = xform.rotation * plane.GetNormal();
            float distanceFromOrigin = plane.mRepr.w - Math::Dot(normalToPlane, xform.transition);
            return BoundingPlane(normalToPlane, distanceFromOrigin);
        }

        // 平面を変換する最も効率的な方法。(1つの四元数ベクトル回転と 1 つのドット積が含まれます。)
        friend BoundingPlane operator* (const Matrix4x4& xform, BoundingPlane plane)
        {
            Vector3 normalToPlane = xform.GetRotation() * plane.GetNormal();
            float distanceFromOrigin = plane.mRepr.w - Math::Dot(normalToPlane, xform.GetTrans());
            return BoundingPlane(normalToPlane, distanceFromOrigin);
        }

    private:

        Vector4 mRepr;
    };


    inline BoundingPlane::BoundingPlane(const Vector3& pointOnPlane, const Vector3& normalToPlane)
    {
        auto nrmPlane = Math::Normalize(normalToPlane);
        mRepr = Vector4(nrmPlane, -Math::Dot(pointOnPlane, nrmPlane));
    }

    inline BoundingPlane PlaneFromPointsCCW(const Vector3& A, const Vector3& B, const Vector3& C)
    {
        return BoundingPlane(A, Math::Cross(C - A, B - A));
    }

}


