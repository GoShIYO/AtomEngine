#pragma once
#include "Runtime/Resource/Model.h"

namespace AtomEngine
{
	struct MaterialComponent
	{
		MaterialComponent(const std::shared_ptr<Model>model = nullptr);


		struct Material
		{
			Color mBaseColor = Color::White;
			Vector3 mEmissive = { 0,0,0 };
			float mMetallic = 0.0f;
			float mRoughness = 1.0f;
			float mNormalScale = 1.0f;
			Vector3 mFresnelF0 = { 0.04f,0.04f,0.04f };
		};
		std::vector<Material> mMaterials;
	};
}
