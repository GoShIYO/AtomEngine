#pragma once
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#define NOMINMAX

namespace AtomEngine
{
    constexpr float aInfinity = std::numeric_limits<float>::infinity();
    constexpr float aNegInfinity = -std::numeric_limits<float>::infinity();
    constexpr float aPI = 3.14159265358979323846264338327950288f;
    constexpr float aOneOverPI = 1.0f / aPI;
    constexpr float aTwoPI = 2.0f * aPI;
    constexpr float aHalfPI = 0.5f * aPI;
    constexpr float deg2Rad = aPI / 180.0f;
    constexpr float rad2Deg = 180.0f / aPI;
    constexpr float aEpsilon = 1e-6f;
    
    class Radian;
    class Angle;
    class Degree;

    class Vector2;
    class Vector3;
    class Vector4;
    class Matrix3x3;
    class Matrix4x4;
    class Quaternion;

    class Radian
    {
        float rad;

    public:
        explicit Radian(float r = 0) : rad(r) {}
        explicit Radian(const Degree& d);
        Radian& operator=(float f)
        {
            rad = f;
            return *this;
        }
        Radian& operator=(const Degree& d);

        float GetRadians() const { return rad; }
        float GetDegrees() const;

        void SetValue(float f) { rad = f; }

        const Radian& operator+() const { return *this; }
        Radian        operator+(const Radian& r) const { return Radian(rad + r.rad); }
        Radian        operator+(const Degree& d) const;
        Radian& operator+=(const Radian& r)
        {
            rad += r.rad;
            return *this;
        }
        Radian& operator+=(const Degree& d);
        Radian  operator-() const { return Radian(-rad); }
        Radian  operator-(const Radian& r) const { return Radian(rad - r.rad); }
        Radian  operator-(const Degree& d) const;
        Radian& operator-=(const Radian& r)
        {
            rad -= r.rad;
            return *this;
        }
        Radian& operator-=(const Degree& d);
        Radian  operator*(float f) const { return Radian(rad * f); }
        Radian  operator*(const Radian& f) const { return Radian(rad * f.rad); }
        Radian& operator*=(float f)
        {
            rad *= f;
            return *this;
        }
        Radian  operator/(float f) const { return Radian(rad / f); }
        Radian& operator/=(float f)
        {
            rad /= f;
            return *this;
        }

        bool operator<(const Radian& r) const { return rad < r.rad; }
        bool operator<=(const Radian& r) const { return rad <= r.rad; }
        bool operator==(const Radian& r) const { return rad == r.rad; }
        bool operator!=(const Radian& r) const { return rad != r.rad; }
        bool operator>=(const Radian& r) const { return rad >= r.rad; }
        bool operator>(const Radian& r) const { return rad > r.rad; }
    };

    class Degree
    {
        float deg;

    public:
        explicit Degree(float d = 0) : deg(d) {}
        explicit Degree(const Radian& r) : deg(r.GetDegrees()) {}
        Degree& operator=(float f)
        {
            deg = f;
            return *this;
        }
        Degree& operator=(const Degree& d) = default;
        Degree& operator=(const Radian& r)
        {
            deg = r.GetDegrees();
            return *this;
        }

        float GetDegrees() const { return deg; }
        float GetRadians() const; // see bottom of this file

        const Degree& operator+() const { return *this; }
        Degree        operator+(const Degree& d) const { return Degree(deg + d.deg); }
        Degree        operator+(const Radian& r) const { return Degree(deg + r.GetDegrees()); }
        Degree& operator+=(const Degree& d)
        {
            deg += d.deg;
            return *this;
        }
        Degree& operator+=(const Radian& r)
        {
            deg += r.GetDegrees();
            return *this;
        }
        Degree  operator-() const { return Degree(-deg); }
        Degree  operator-(const Degree& d) const { return Degree(deg - d.deg); }
        Degree  operator-(const Radian& r) const { return Degree(deg - r.GetDegrees()); }
        Degree& operator-=(const Degree& d)
        {
            deg -= d.deg;
            return *this;
        }
        Degree& operator-=(const Radian& r)
        {
            deg -= r.GetDegrees();
            return *this;
        }
        Degree  operator*(float f) const { return Degree(deg * f); }
        Degree  operator*(const Degree& f) const { return Degree(deg * f.deg); }
        Degree& operator*=(float f)
        {
            deg *= f;
            return *this;
        }
        Degree  operator/(float f) const { return Degree(deg / f); }
        Degree& operator/=(float f)
        {
            deg /= f;
            return *this;
        }

        bool operator<(const Degree& d) const { return deg < d.deg; }
        bool operator<=(const Degree& d) const { return deg <= d.deg; }
        bool operator==(const Degree& d) const { return deg == d.deg; }
        bool operator!=(const Degree& d) const { return deg != d.deg; }
        bool operator>=(const Degree& d) const { return deg >= d.deg; }
        bool operator>(const Degree& d) const { return deg > d.deg; }
    };

    class Math
    {
    public:

        static float Abs(float value) { return std::fabs(value); }
        static bool  IsNaN(float f) { return std::isnan(f); }
        static float Sqr(float value) { return value * value; }
        static float Sqrt(float fValue) { return std::sqrt(fValue); }
        static float InvSqrt(float value) { return 1.f / Sqrt(value); }
        static bool  Equal(float a, float b, float tolerance = std::numeric_limits<float>::epsilon());
        static float Clamp(float v, float min, float max) { return std::clamp(v, min, max); }
        static float MaxElement(float x, float y, float z) { return std::max({ x, y, z }); }

        static float DegreesToRadians(float degrees);
        static float RadiansToDegrees(float radians);

        static float  sin(const Radian& rad) { return std::sin(rad.GetRadians()); }
        static float  sin(float value) { return std::sin(value); }
        static float  cos(const Radian& rad) { return std::cos(rad.GetRadians()); }
        static float  cos(float value) { return std::cos(value); }
        static float  tan(const Radian& rad) { return std::tan(rad.GetRadians()); }
        static float  tan(float value) { return std::tan(value); }
        static Radian acos(float value);
        static Radian asin(float value);
        static Radian atan(float value) { return Radian(std::atan(value)); }
        static Radian atan2(float y_v, float x_v) { return Radian(std::atan2(y_v, x_v)); }

        template<class T>
        static constexpr T max(const T A, const T B)
        {
            return std::max(A, B);
        }

        template<class T>
        static constexpr T min(const T A, const T B)
        {
            return std::min(A, B);
        }

        template<class T>
        static constexpr T max3(const T A, const T B, const T C)
        {
            return std::max({ A, B, C });
        }

        template<class T>
        static constexpr T min3(const T A, const T B, const T C)
        {
            return std::min({ A, B, C });
        }

        static Matrix4x4
            MakeViewMatrix(const Vector3& position, const Quaternion& orientation, const Matrix4x4* reflectMatrix = nullptr);

        static Matrix4x4
            MakeLookAtMatrix(const Vector3& eyePosition, const Vector3& targetPosition, const Vector3& upDir);

        static Matrix4x4 MakePerspectiveMatrix(Radian fovy, float aspect, float znear = 0.1f, float zfar = 100.0f);

        static Matrix4x4
            MakeOrthographicProjectionMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);
    };

    inline Radian::Radian(const Degree& d) : rad(d.GetRadians()) {}
    inline Radian& Radian::operator=(const Degree& d)
    {
        rad = d.GetRadians();
        return *this;
    }
    inline Radian Radian::operator+(const Degree& d) const { return Radian(rad + d.GetRadians()); }
    inline Radian& Radian::operator+=(const Degree& d)
    {
        rad += d.GetRadians();
        return *this;
    }
    inline Radian Radian::operator-(const Degree& d) const { return Radian(rad - d.GetRadians()); }
    inline Radian& Radian::operator-=(const Degree& d)
    {
        rad -= d.GetRadians();
        return *this;
    }

    inline float Radian::GetDegrees() const { return Math::RadiansToDegrees(rad); }

    inline float Degree::GetRadians() const { return Math::DegreesToRadians(deg); }

    inline Radian operator*(float a, const Radian& b) { return Radian(a * b.GetRadians()); }

    inline Radian operator/(float a, const Radian& b) { return Radian(a / b.GetRadians()); }

    inline Degree operator*(float a, const Degree& b) { return Degree(a * b.GetDegrees()); }

    inline Degree operator/(float a, const Degree& b) { return Degree(a / b.GetDegrees()); }
}


