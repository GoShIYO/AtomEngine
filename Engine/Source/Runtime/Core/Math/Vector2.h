#pragma once
#include "Math.h"

namespace AtomEngine
{
	class Vector2
	{
	public:
		float x{ 0.0f }, y{ 0.0f };
	public:
		Vector2() = default;
		Vector2(float x_, float y_) : x(x_), y(y_) {}
		float Distance(const Vector2& other) const { return std::sqrt((x - other.x) * (x - other.x) + (y - other.y) * (y - other.y)); }
		float Length() const { return sqrtf(LengthSqr()); }
		float LengthSqr() const { return x * x + y * y; }
		Vector2 Normalize() const
		{
			float len = Length();
			if (len == 0)
			{
				return Vector2{ 0, 0 };
			}
			return Vector2{ x / len, y / len };
		}
		float Dot(const Vector2& other) const { return x * other.x + y * other.y; }
		Vector2 operator+(const Vector2& other) const { return { x + other.x, y + other.y }; }

		Vector2 operator-(const Vector2& other) const { return { x - other.x, y - other.y }; }
		Vector2 operator-() { return { -x, -y }; }
		Vector2& operator+=(const Vector2& other)
		{
			x += other.x;
			y += other.y;
			return *this;
		}
		Vector2& operator+=(const float& other)
		{
			x += other;
			y += other;
			return *this;
		}
		Vector2& operator-=(const Vector2& other)
		{
			x -= other.x;
			y -= other.y;
			return *this;
		}
		Vector2& operator-=(const float& other)
		{
			x -= other;
			y -= other;
			return *this;
		}
		Vector2 operator*(float a) const { return { x * a, y * a }; }
		Vector2 operator/(float a) const
		{
			if (a == 0.0f)
			{
				return { 0.0f, 0.0f };
			}
			return { x / a, y / a };
		}
		Vector2 operator/(const Vector2& other)
		{
			if (other.x == 0.0f || other.y == 0.0f)
			{
				return { 0.0f, 0.0f };
			}
			return { x / other.x, y / other.y };
		}
		float operator*(const Vector2& other) const { return x * other.x + y * other.y; }
		void operator*=(const float value) { x *= value, y *= value; }

		static const Vector2 ZERO;
	};
}


