#pragma once
#include "Runtime/Core/Math/Vector3.h"
#include "Runtime/Core/Math/Vector4.h"

namespace AtomEngine
{
	/// <summary>
	/// カラークラス(デフォルト色空間はLinearColorSpace)
	/// </summary>
	class Color
	{
	public:
		float r, g, b, a;

		Vector3 ToVector3() const
		{
			return Vector3(r, g, b);
		}
		Vector4 ToVector4() const
		{
			return Vector4(r, g, b, a);
		}
		
		Color() = default;
        Color(float r_, float g_, float b_, float a_) : r(r_), g(g_), b(b_), a(a_) {}
		Color(float r_, float g_, float b_) : r(r_), g(g_), b(b_), a(1.0f) {}
        Color(const Vector3& v) : r(v.x), g(v.y), b(v.z), a(1.0f) {}
        Color(const Vector4& v) : r(v.x), g(v.y), b(v.z), a(v.w) {}

		float* ptr(void) { return reinterpret_cast<float*>(this); }
		float& operator[](int idx) { return ptr()[idx]; }

		Color(uint32_t rgba)
		{
			r = ((rgba >> 24) & 0xFF) / 255.0f;
			g = ((rgba >> 16) & 0xFF) / 255.0f;
			b = ((rgba >> 8) & 0xFF) / 255.0f;
			a = ((rgba >> 0) & 0xFF) / 255.0f;
		}

		//SRGB
		Color ToSRGB();
		Color FromSRGB();

		//HSV
		Color ToHSV();
		Color FromHSV();

		//十六進数に変換
		uint32_t R10G10B10A2() const;
		uint32_t R8G8B8A8() const;

	public:
		 static const Color Black;
		 static const Color White;
		 static const Color Magenta;
		 static const Color Red;
		 static const Color Green;
		 static const Color Blue;
		 static const Color Yellow;
		 static const Color Cyan;
		 static const Color Gray;
	};
	
	inline Color Max(const Color& a, const Color& b)
	{
		return Color(
			Math::Max(a.r, b.r),
			Math::Max(a.g, b.g),
			Math::Max(a.b, b.b),
			Math::Max(a.a, b.a)
		);
	}
	inline Color Min(const Color& a, const Color& b)
	{
		return Color(
			Math::Min(a.r, b.r),
			Math::Min(a.g, b.g),
			Math::Min(a.b, b.b),
			Math::Min(a.a, b.a)
		);
	}
	inline Color Clamp(const Color& color, const Color& min, const Color& max)
	{
		return Color(
			std::clamp(color.r, min.r, max.r),
			std::clamp(color.g, min.g, max.g),
			std::clamp(color.b, min.b, max.b),
			std::clamp(color.a, min.a, max.a)
		);
	}
}
