#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include<array>

namespace AtomEngine
{
    enum TextureSlot : uint8_t
    {
        kBaseColor          = 1 << 0,
        kMetallic           = 1 << 1,
        kRoughness          = 1 << 2,
        kNormal             = 1 << 3,
        kOcclusion          = 1 << 4,
        kEmissive           = 1 << 5,
        kMetallicRoughness  = 1 << 6,
    };

    struct MaterialTexture
    {
        std::wstring name;
        TextureSlot slot;
    };

	struct Material
	{
        std::string name;

		Vector4 baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector3 emissiveFactor = { 0.0f, 0.0f, 0.0f };
		float metallicFactor = 0.0f;
		float roughnessFactor = 0.0f;
	
        uint8_t textureMask = 0;

        float transmissionFactor = 0.0f;
        bool hasTransmissionTexture = false;
        bool hasAlphaBlend = false;

        std::vector<MaterialTexture> textures;
    };
}

