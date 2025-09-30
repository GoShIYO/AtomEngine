#pragma once
#include "Runtime/Core/Math/MathInclude.h"

namespace AtomEngine
{
	enum
	{
		kBaseColor,
		kMetallicRoughness,
		kOcclusion,
		kEmissive,
		kNormal,

		kNumTextures
	};

	struct MaterialConstants
	{
		float baseColorFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		float emissiveFactor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float metallicFactor = 0.0f;
		float roughnessFactor = 0.0f;
		uint32_t flags = 0;
	};

	struct MaterialTexture
	{
		uint16_t stringIdx[kNumTextures];
		uint16_t addressModes;
	};

}

