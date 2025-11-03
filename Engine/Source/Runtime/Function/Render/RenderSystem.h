#pragma once
#include "Runtime/Platform/DirectX12/Core/DescriptorHeap.h"
#include "Runtime/Platform/DirectX12/Pipline/PiplineState.h"
#include "Runtime/Resource/TextureRef.h"
#include "Runtime/Function/Camera/CameraBase.h"
namespace AtomEngine
{
	enum RootBindings
	{
		kMeshConstants,			// メッシュ定数バッファ
		kMaterialConstants,		// マテリアル定数バッファ
		kMaterialSRVs,			// テクスチャ
		kCommonSRVs,			// 共通テクスチャ(depth 環境mapとか)
		kCommonCBV,				// グローバル定数バッファ(カメラ ライト等)
		kSkinMatrices,			// スキニング行列バッファ

		kNumRootBindings
	};

	enum class PSOIndex : uint16_t
	{
		kPSO_Default,
		kPSO_Skin,
		kPSO_MRWorkFlow,
		kPSO_MRWorkFlow_Skin,
		kPSO_Transparent,
		kPSO_Transparent_Skin,
		kPSO_Transparent_MRWorkFlow,
        kPSO_Transparent_MRWorkFlow_Skin,
		kPSO_DepthOnly,
		kPSO_DepthOnly_Skin,
		kPSO_Shadow,
        kPSO_Shadow_Skin,

		kNumPSOs
	};

	class Renderer
	{
	public:

		static void Initialize();

		static void Update(float deltaTime);

		static void Render(float deltaTime);

		static void Shutdown();

		static DescriptorHeap& GetTextureHeap() { return gTextureHeap; }
        static DescriptorHandle& GetCommonTextures() { return gCommonTextures; }

		static const RootSignature& GetRootSignature() { return mRootSig; }
		static const GraphicsPSO& GetPSO(uint16_t psoFlags);
	private:
		static std::vector<GraphicsPSO> gPSOs;
		static RootSignature mRootSig;
		static DescriptorHeap gTextureHeap;
		static DescriptorHandle gCommonTextures;
	};
}
