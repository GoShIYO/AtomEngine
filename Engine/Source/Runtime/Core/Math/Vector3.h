#pragma once
#include "Math.h"
#include "Quaternion.h"

namespace AtomEngine
{
	class Vector3
	{
	public:
		float x = 0.0f, y = 0.0f, z = 0.0f;

	public:
        Vector3() = default;
        Vector3(float x_, float y_, float z_) : x{ x_ }, y{ y_ }, z{ z_ } {}

        explicit Vector3(const float coords[3]) : x{ coords[0] }, y{ coords[1] }, z{ coords[2] } {}

        float operator[](size_t i) const
        {
            assert(i < 3);
            return *(&x + i);
        }

        float& operator[](size_t i)
        {
            assert(i < 3);
            return *(&x + i);
        }

        float* ptr() { return &x; }

        const float* ptr() const { return &x; }

        bool operator==(const Vector3& rhs) const { return (x == rhs.x && y == rhs.y && z == rhs.z); }

        bool operator!=(const Vector3& rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z; }

        Vector3 operator+(const Vector3& rhs) const { return Vector3(x + rhs.x, y + rhs.y, z + rhs.z); }

        Vector3 operator-(const Vector3& rhs) const { return Vector3(x - rhs.x, y - rhs.y, z - rhs.z); }

        Vector3 operator*(float scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }

        Vector3 operator*(const Vector3& rhs) const { return Vector3(x * rhs.x, y * rhs.y, z * rhs.z); }

        Vector3 operator/(float scalar) const
        {
            assert(scalar != 0.0);
            return Vector3(x / scalar, y / scalar, z / scalar);
        }

        Vector3 operator/(const Vector3& rhs) const
        {
            assert((rhs.x != 0 && rhs.y != 0 && rhs.z != 0));
            return Vector3(x / rhs.x, y / rhs.y, z / rhs.z);
        }

        const Vector3& operator+() const { return *this; }

        Vector3 operator-() const { return Vector3(-x, -y, -z); }

        friend Vector3 operator*(float scalar, const Vector3& rhs)
        {
            return Vector3(scalar * rhs.x, scalar * rhs.y, scalar * rhs.z);
        }

        friend Vector3 operator/(float scalar, const Vector3& rhs)
        {
            assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
            return Vector3(scalar / rhs.x, scalar / rhs.y, scalar / rhs.z);
        }

        friend Vector3 operator+(const Vector3& lhs, float rhs)
        {
            return Vector3(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs);
        }

        friend Vector3 operator+(float lhs, const Vector3& rhs)
        {
            return Vector3(lhs + rhs.x, lhs + rhs.y, lhs + rhs.z);
        }

        friend Vector3 operator-(const Vector3& lhs, float rhs)
        {
            return Vector3(lhs.x - rhs, lhs.y - rhs, lhs.z - rhs);
        }

        friend Vector3 operator-(float lhs, const Vector3& rhs)
        {
            return Vector3(lhs - rhs.x, lhs - rhs.y, lhs - rhs.z);
        }

        Vector3& operator+=(const Vector3& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }

        Vector3& operator+=(float scalar)
        {
            x += scalar;
            y += scalar;
            z += scalar;
            return *this;
        }

        Vector3& operator-=(const Vector3& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            return *this;
        }

        Vector3& operator-=(float scalar)
        {
            x -= scalar;
            y -= scalar;
            z -= scalar;
            return *this;
        }

        Vector3& operator*=(float scalar)
        {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }

        Vector3& operator*=(const Vector3& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            return *this;
        }

        Vector3& operator/=(float scalar)
        {
            assert(scalar != 0.0);
            x /= scalar;
            y /= scalar;
            z /= scalar;
            return *this;
        }

        Vector3& operator/=(const Vector3& rhs)
        {
            assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            return *this;
        }

        float Length() const { return std::hypot(x, y, z); }

        float LengthSqr() const { return x * x + y * y + z * z; }

        float Distance(const Vector3& rhs) const { return (*this - rhs).Length(); }

        float DistanceSqr(const Vector3& rhs) const { return (*this - rhs).LengthSqr(); }

        float Dot(const Vector3& vec) const { return x * vec.x + y * vec.y + z * vec.z; }

        void Normalize()
        {
            float length = std::hypot(x, y, z);
            if (length == 0.f)
                return;

            float inv_lengh = 1.0f / length;
            x *= inv_lengh;
            y *= inv_lengh;
            z *= inv_lengh;
        }

        Vector3 Cross(const Vector3& rhs) const
        {
            return Vector3(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
        }

        void Floor(const Vector3& cmp)
        {
            if (cmp.x < x)
                x = cmp.x;
            if (cmp.y < y)
                y = cmp.y;
            if (cmp.z < z)
                z = cmp.z;
        }

        void Ceil(const Vector3& cmp)
        {
            if (cmp.x > x)
                x = cmp.x;
            if (cmp.y > y)
                y = cmp.y;
            if (cmp.z > z)
                z = cmp.z;
        }

        Radian AngleBetween(const Vector3& dest) const
        {
            float len_product = Length() * dest.Length();

            if (len_product < 1e-6f)
                len_product = 1e-6f;

            float f = Dot(dest) / len_product;

            f = Math::Clamp(f, (float)-1.0, (float)1.0);
            return Math::acos(f);
        }

        Quaternion GetRotationTo(const Vector3& dest, const Vector3& fallback_axis = Vector3::ZERO) const
        {
            Quaternion q;

            Vector3 v0 = *this;
            Vector3 v1 = dest;
            v0.Normalize();
            v1.Normalize();

            float d = v0.Dot(v1);
            // If dot == 1, vectors are the same
            if (d >= 1.0f)
            {
                return Quaternion::IDENTITY;
            }
            if (d < (1e-6f - 1.0f))
            {
                if (fallback_axis != Vector3::ZERO)
                {
                    q.FromAngleAxis(Radian(aPI), fallback_axis);
                }
                else
                {
                    Vector3 axis = Vector3::RIGHT.Cross(*this);
                    if (axis.IsZeroLength())
                        axis = Vector3::UP.Cross(*this);
                    axis.Normalize();
                    q.FromAngleAxis(Radian(aPI), axis);
                }
            }
            else
            {
                float s = Math::Sqrt((1 + d) * 2);
                float invs = 1 / s;

                Vector3 c = v0.Cross(v1);

                q.x = c.x * invs;
                q.y = c.y * invs;
                q.z = c.z * invs;
                q.w = s * 0.5f;
                q.Normalize();
            }
            return q;
        }

        bool IsZeroLength(void) const
        {
            float sqlen = (x * x) + (y * y) + (z * z);
            return (sqlen < (1e-06 * 1e-06));
        }

        bool IsZero() const { return x == 0.f && y == 0.f && z == 0.f; }

        Vector3 NormalizedCopy(void) const
        {
            Vector3 ret = *this;
            ret.Normalize();
            return ret;
        }

        Vector3 reflect(const Vector3& normal) const
        {
            return Vector3(*this - (2 * this->Dot(normal) * normal));
        }

        Vector3 project(const Vector3& normal) const { return Vector3(*this - (this->Dot(normal) * normal)); }

        Vector3 absoluteCopy() const { return Vector3(std::fabsf(x), std::fabsf(y), std::fabsf(z)); }

        static Vector3 Lerp(const Vector3& lhs, const Vector3& rhs, float alpha) { return lhs + alpha * (rhs - lhs); }

        static Vector3 clamp(const Vector3& v, const Vector3& min, const Vector3& max)
        {
            return Vector3(
                Math::Clamp(v.x, min.x, max.x), Math::Clamp(v.y, min.y, max.y), Math::Clamp(v.z, min.z, max.z));
        }

        bool         IsNaN() const { return Math::IsNaN(x) || Math::IsNaN(y) || Math::IsNaN(z); }

        static const Vector3 ZERO;
        static const Vector3 RIGHT;
        static const Vector3 UP;
        static const Vector3 FORWARD;
        static const Vector3 UNIT_SCALE;
	};
}
