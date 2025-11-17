#pragma once
#include "../Sprite/Sprite.h"
#include <vector>

namespace AtomEngine
{
	struct SpriteConstants
	{
		Matrix4x4 viewProj;
		Matrix4x4 uvTransform;
		Vector4 color;
	};
	class SpriteRenderer
	{
	public:

		static void Initialize();
		static void Shutdown();

		static void AddSprite(
			const Sprite* sprite,
			const TextureRef& texture,
			BlendMode blendMode);

		static void Sort();

		static void Render(GraphicsContext& gfxContext);
	private:

		struct SpriteItem
		{
			const Sprite* sprite;
			D3D12_CPU_DESCRIPTOR_HANDLE textureHandle;
			BlendMode blendMode;
			uint64_t sortKey;
		};

		static std::vector<SpriteVertex>sVertices;
		static std::vector<uint16_t>sIndices;

		static std::vector<SpriteItem>sSpriteItems;

		static RootSignature sRootSig;

		static GraphicsPSO sPSO[kNumBlendModes];
	};
}
