#pragma once
#include <functional>
#include "../Runtime/Core/Math/MathInclude.h"


//sin(θ(1−t))sin(θ)
namespace Easing
{
	template<typename T>
	static inline T Lerp(const T& st_, const T& end_, float t_)
	{
		return end_ * t_ + st_ * (1.0f - t_);
	}

	template<typename T>
	static inline T EasingInBack(const T& st_, const T& end_, float t_)
	{
		float const c1 = 1.70158f;
		float const c3 = c1 + 1.0f;

		float convertedT = c3 * t_ * t_ * t_ - c1 * t_ * t_;

		return Lerp(st_, end_, convertedT);
	}

	template<typename T>
	static inline T EaseOutBack(const T& st_, const T& end_, float t_)
	{
		float const c1 = 1.70158f;
		float const c3 = c1 + 1.0f;

		float convertedT = 1.0f + c3 * powf(t_ - 1.0f, 3.0f) + c1 * powf(t_ - 1.0f, 2.0f);

		return Lerp(st_, end_, convertedT);
	}

	template<typename T>
	static inline T EaseOutCubic(const T& st_, const T& end_, float t_)
	{
		float convertedT = 1.0f - (float)pow(1.0f - t_, 3.0f);

		return Lerp(st_, end_, convertedT);
	}

	template<typename T>
	static inline T EaseInExpo(const T& st_, const T& end_, float t_)
	{
		float convertedT = (float)powf(2.0f, 10.0f * t_ - 10.0f);

		return Lerp(st_, end_, convertedT);
	}

	template<typename T>
	static inline T EaseOutBounce(const T& st_, const T& end_, float t_)
	{
		float const n1 = 7.5625f;
		float const d1 = 2.75f;

		float convertedT;

		if (t_ < 1.0f / d1)
		{
			convertedT = n1 * t_ * t_;
		}

		else if (t_ < 2.0f / d1)
		{
			convertedT = n1 * (t_ -= 1.5f / d1 )* t_ +0.75f;
		}

		else if (t_ < 2.5f / d1)
		{
			convertedT = n1 * (t_ -= 2.25f / d1) * t_ + 0.9375f;
		}

		else
		{
			convertedT = n1 * (t_ -= 2.625f / d1) * t_ + 0.984375f;
		}

		return Lerp(st_, end_, convertedT);
	}

	template<typename T>
	static inline T EaseOutElastic(const T& st_, const T& end_, float t_)
	{
		float convertedT;
		static float const c4 = (2 * 3.14159265359f) / 3.0f;
		convertedT	= powf(2.0f, -10.0f * t_)* sinf((t_ * 10.0f - 0.75f) * c4) + 1.0f;

		return Lerp(st_, end_, convertedT);
	}

	template<typename T>
	static inline T EaseOutExpo(const T& st_, const T& end_, float t_)
	{
		float convertedT;
		convertedT = 1.0f - powf(2.0f, -10.0f * t_);

		return Lerp(st_, end_, convertedT);

	}

	template<typename T>
	static inline T EaseOutQuint(const T& st_, const T& end_, float t_)
	{
		float convertedT;
		convertedT = 1.0f - powf(1.0f - t_, 5.0f);

		return Lerp(st_, end_, convertedT);

	}


}

