#pragma once
#include "Runtime/Platform/DirectX12/Core/DescriptorHeap.h"
#include "Runtime/Platform/DirectX12/Pipline/PiplineState.h"
#include "Runtime/Resource/TextureManager.h"

namespace AtomEngine
{
	enum RootBindings
	{
		kMeshConstants,			// メッシュ定数バッファ
		kMaterialConstants,		// マテリアル定数バッファ
		kMaterialSRVs,			// テクスチャ
		kMaterialSamplers,		// サンプラー
		kCommonSRVs,			// 共通テクスチャ(depth 環境mapとか)
		kCommonCBV,				// グローバル定数バッファ(カメラ ライト等)
		kSkinMatrices,			// スキニング行列バッファ

		kNumRootBindings
	};

	class RenderSystem
	{
	public:

		static void Initialize();

		static void Update();

		static void Render(float deltaTime);

		static void Shutdown();

		static DescriptorHeap GetTextureHeap() { return gTextureHeap; }
		static DescriptorHeap GetSamplerHeap() { return gSamplerHeap; }
        static DescriptorHandle GetCommonTextures() { return gCommonTextures; }

		static const RootSignature& GetRootSignature() { return gRootSig; }

		static uint8_t GetPSO(uint16_t psoFlags);
		static void SetIBLTextures(TextureRef diffuseIBL, TextureRef specularIBL);
		static void SetIBLBias(float LODBias);
		static float GetIBLRange();
		static float GetIBLBias();
		static const PSO& GetPSO(uint64_t index);
	private:
		static std::vector<GraphicsPSO> gPSOs;
		static RootSignature gRootSig;
		static DescriptorHeap gTextureHeap;
		static DescriptorHeap gSamplerHeap;
		static DescriptorHandle gCommonTextures;
	};
}


