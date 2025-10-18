#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include<array>

namespace AtomEngine
{
    enum class TextureSlot : uint8_t
    {
        kBaseColor,
        kMetallic,
        kRoughness,
        kNormal,
        kOcclusion,
        kEmissive,
        kMetallicRoughness,

        kNumTextures
    };

    enum PSOFlags : uint16_t
    {
        kHasSkin = 1 << 0,
        //TODO:alpha test / alpha blend
    };

    struct MaterialTexture
    {
        std::wstring name;
        TextureSlot slot;
    };

	struct Material
	{
        std::string name;

		float baseColorFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		float emissiveFactor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float metallicFactor = 0.0f;
		float roughnessFactor = 0.0f;
	
        uint32_t textureMask = 0;
        std::vector<MaterialTexture> textures;
    };
}

