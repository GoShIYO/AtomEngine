#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"

namespace AtomEngine
{
    class BoundingSphere
    {
    public:
        BoundingSphere() {}
        BoundingSphere(float x, float y, float z, float r) : mRepr(x, y, z, r) {}
        BoundingSphere(const Vector4& v4) : mRepr(v4) {}
        BoundingSphere(const Vector3& center, float radius);

        explicit BoundingSphere(const Vector3& f3) : mRepr(Vector4(f3,1.0f)) {}
        explicit operator Vector4() const { return Vector4(mRepr); }

        Vector3 GetCenter(void) const { return Vector3(mRepr); }
        float GetRadius(void) const { return mRepr.w; }

        BoundingSphere Union(const BoundingSphere& rhs);

    private:

        Vector4 mRepr;
    };

    inline BoundingSphere::BoundingSphere(const Vector3& center, float radius)
    {
        mRepr = Vector4(center,0.0f);
        mRepr.w = radius;
    }
}


