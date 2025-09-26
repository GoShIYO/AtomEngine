#include "Color.h"

namespace AtomEngine
{
	const Color Color::Black(0.0f, 0.0f, 0.0f, 1.0f);
	const Color Color::White(1.0f, 1.0f, 1.0f, 1.0f);
	const Color Color::Magenta(1.0f, 0.0f, 1.0f, 1.0f);
	const Color Color::Red(1.0f, 0.0f, 0.0f, 1.0f);
	const Color Color::Green(0.0f, 1.0f, 0.0f, 1.0f);
	const Color Color::Blue(0.0f, 0.0f, 1.0f, 1.0f);
	const Color Color::Yellow(1.0f, 1.0f, 0.0f, 1.0f);
	const Color Color::Cyan(0.0f, 1.0f, 1.0f, 1.0f);
	const Color Color::Gray(0.5f, 0.5f, 0.5f, 1.0f);

	Color AtomEngine::Color::ToSRGB()
	{
		auto linearToSRGB = [](float c)
			{
				return (c <= 0.0031308f) ? (12.92f * c) : (1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f);
			};
		return Color(
			linearToSRGB(r),
			linearToSRGB(g),
			linearToSRGB(b),
			a
		);
	}

	Color AtomEngine::Color::FromSRGB()
	{
		auto sRGBToLinear = [](float c)
			{
				return (c <= 0.04045f) ? (c / 12.92f) : (std::pow((c + 0.055f) / 1.055f, 2.4f));
			};
		return Color(
			sRGBToLinear(r),
			sRGBToLinear(g),
			sRGBToLinear(b),
			a
		);
	}

	Color Color::ToHSV()
	{
		float R = std::clamp(r, 0.0f, 1.0f);
		float G = std::clamp(g, 0.0f, 1.0f);
		float B = std::clamp(b, 0.0f, 1.0f);

		float maxC = std::max({ R, G, B });
		float minC = std::min({ R, G, B });
		float delta = maxC - minC;

		float H = 0.0f;
		if (delta > 1e-6f)
		{
			if (maxC == R)
				H = 60.0f * fmod(((G - B) / delta), 6.0f);
			else if (maxC == G)
				H = 60.0f * (((B - R) / delta) + 2.0f);
			else
				H = 60.0f * (((R - G) / delta) + 4.0f);
		}
		if (H < 0.0f) H += 360.0f;

		float S = (maxC <= 1e-6f) ? 0.0f : (delta / maxC);
		float V = maxC;

		return Color(H, S, V, a);
	}

	Color Color::FromHSV()
	{
		float H = r; // Hue in [0,360]
		float S = g; // Saturation [0,1]
		float V = b; // Value [0,1]

		float C = V * S;
		float X = C * (1.0f - std::fabsf(std::fmod(H / 60.0f, 2.0f) - 1.0f));
		float m = V - C;

		float R = 0, G = 0, B = 0;

		if (H >= 0 && H < 60)
			R = C, G = X, B = 0;
		else if (H >= 60 && H < 120)
			R = X, G = C, B = 0;
		else if (H >= 120 && H < 180)
			R = 0, G = C, B = X;
		else if (H >= 180 && H < 240)
			R = 0, G = X, B = C;
		else if (H >= 240 && H < 300)
			R = X, G = 0, B = C;
		else if (H >= 300 && H < 360)
			R = C, G = 0, B = X;

		return Color(R + m, G + m, B + m, a);
	}

	uint32_t AtomEngine::Color::R10G10B10A2() const
	{
		uint32_t R = static_cast<uint32_t>(std::clamp(r, 0.0f, 1.0f) * 1023.0f + 0.5f);
		uint32_t G = static_cast<uint32_t>(std::clamp(g, 0.0f, 1.0f) * 1023.0f + 0.5f);
		uint32_t B = static_cast<uint32_t>(std::clamp(b, 0.0f, 1.0f) * 1023.0f + 0.5f);
		uint32_t A = static_cast<uint32_t>(std::clamp(a, 0.0f, 1.0f) * 3.0f + 0.5f);

		return (R << 22) | (G << 12) | (B << 2) | A;
	}

	uint32_t AtomEngine::Color::R8G8B8A8() const
	{
		uint32_t R = static_cast<uint32_t>(std::clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
		uint32_t G = static_cast<uint32_t>(std::clamp(g, 0.0f, 1.0f) * 255.0f + 0.5f);
		uint32_t B = static_cast<uint32_t>(std::clamp(b, 0.0f, 1.0f) * 255.0f + 0.5f);
		uint32_t A = static_cast<uint32_t>(std::clamp(a, 0.0f, 1.0f) * 255.0f + 0.5f);

		return (R << 24) | (G << 16) | (B << 8) | A;
	}
}

