#include "RenderSystem.h"

#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"
#include "Runtime/Platform/DirectX12/Pipline/PiplineState.h"
#include "Runtime/Platform/DirectX12/Buffer/ColorBuffer.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Resource/Model.h"
#include "Runtime/Core/Utility/Utility.h"

#include "imgui.h"

namespace AtomEngine
{
	TextureRef test;
	DescriptorHandle gpuHandle;

	std::vector<GraphicsPSO> RenderSystem::gPSOs;
	RootSignature RenderSystem::gRootSig;
	DescriptorHeap RenderSystem::gTextureHeap;
	DescriptorHeap RenderSystem::gSamplerHeap;
	DescriptorHandle RenderSystem::gCommonTextures;

	struct Skybox
	{
		TextureRef RadianceCubeMap;
		TextureRef IrradianceCubeMap;
		float SpecularIBLRange;
		float SpecularIBLBias;
	};
	Skybox skybox;

	//uint32_t SSAOFullScreenID;
	uint32_t ShadowBufferID;

	GraphicsPSO m_SkyboxPSO(L"Renderer: Skybox PSO");
	GraphicsPSO m_DefaultPSO(L"Renderer: Default PSO"); // Not finalized.  Used as a template.

	void RenderSystem::Initialize()
	{
		//SamplerDesc DefaultSamplerDesc;
		//DefaultSamplerDesc.MaxAnisotropy = 8;

		//SamplerDesc CubeMapSamplerDesc = DefaultSamplerDesc;
		////CubeMapSamplerDesc.MaxLOD = 6.0f;

		//gRootSig.Reset(kNumRootBindings, 3);

		//gRootSig.InitStaticSampler(10, DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		//gRootSig.InitStaticSampler(11, SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		//gRootSig.InitStaticSampler(12, CubeMapSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);

		//gRootSig[kMeshConstants].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
		//gRootSig[kMaterialConstants].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);
		//gRootSig[kMaterialSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10, D3D12_SHADER_VISIBILITY_PIXEL);
		//gRootSig[kMaterialSamplers].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 10, D3D12_SHADER_VISIBILITY_PIXEL);
		//gRootSig[kCommonSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 10, D3D12_SHADER_VISIBILITY_PIXEL);
		//gRootSig[kCommonCBV].InitAsConstantBuffer(1);
		//gRootSig[kSkinMatrices].InitAsBufferSRV(20, D3D12_SHADER_VISIBILITY_VERTEX);

		//gRootSig.Finalize(L"RootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		//DXGI_FORMAT ColorFormat = gSceneColorBuffer.GetFormat();
		//DXGI_FORMAT DepthFormat = gSceneDepthBuffer.GetFormat();

		//D3D12_INPUT_ELEMENT_DESC posOnly[] =
		//{
		//    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//};

		//D3D12_INPUT_ELEMENT_DESC posAndUV[] =
		//{
		//    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//    { "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//};

		//D3D12_INPUT_ELEMENT_DESC skinPos[] =
		//{
		//    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//    { "BLENDINDICES", 0, DXGI_FORMAT_R16G16B16A16_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//    { "BLENDWEIGHT", 0, DXGI_FORMAT_R16G16B16A16_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//};

		//D3D12_INPUT_ELEMENT_DESC skinPosAndUV[] =
		//{
		//    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//    { "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//    { "BLENDINDICES", 0, DXGI_FORMAT_R16G16B16A16_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//    { "BLENDWEIGHT", 0, DXGI_FORMAT_R16G16B16A16_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//};

		//ASSERT(gPSOs.size() == 0);

		//// Depth Only PSOs
		//{
		//    GraphicsPSO DepthOnlyPSO(L"Renderer: Depth Only PSO");
		//    DepthOnlyPSO.SetRootSignature(gRootSig);
		//    DepthOnlyPSO.SetRasterizerState(RasterizerDefault);
		//    DepthOnlyPSO.SetBlendState(BlendDisable);
		//    DepthOnlyPSO.SetDepthStencilState(DepthStateReadWrite);
		//    DepthOnlyPSO.SetInputLayout(_countof(posOnly), posOnly);
		//    DepthOnlyPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//    DepthOnlyPSO.SetRenderTargetFormats(0, nullptr, DepthFormat);
		//    DepthOnlyPSO.SetVertexShader(g_pDepthOnlyVS, sizeof(g_pDepthOnlyVS));
		//    DepthOnlyPSO.Finalize();
		//    gPSOs.push_back(DepthOnlyPSO);

		//    GraphicsPSO CutoutDepthPSO(L"Renderer: Cutout Depth PSO");
		//    CutoutDepthPSO = DepthOnlyPSO;
		//    CutoutDepthPSO.SetInputLayout(_countof(posAndUV), posAndUV);
		//    CutoutDepthPSO.SetRasterizerState(RasterizerTwoSided);
		//    CutoutDepthPSO.SetVertexShader(g_pCutoutDepthVS, sizeof(g_pCutoutDepthVS));
		//    CutoutDepthPSO.SetPixelShader(g_pCutoutDepthPS, sizeof(g_pCutoutDepthPS));
		//    CutoutDepthPSO.Finalize();
		//    gPSOs.push_back(CutoutDepthPSO);

		//    GraphicsPSO SkinDepthOnlyPSO = DepthOnlyPSO;
		//    SkinDepthOnlyPSO.SetInputLayout(_countof(skinPos), skinPos);
		//    SkinDepthOnlyPSO.SetVertexShader(g_pDepthOnlySkinVS, sizeof(g_pDepthOnlySkinVS));
		//    SkinDepthOnlyPSO.Finalize();
		//    gPSOs.push_back(SkinDepthOnlyPSO);

		//    GraphicsPSO SkinCutoutDepthPSO = CutoutDepthPSO;
		//    SkinCutoutDepthPSO.SetInputLayout(_countof(skinPosAndUV), skinPosAndUV);
		//    SkinCutoutDepthPSO.SetVertexShader(g_pCutoutDepthSkinVS, sizeof(g_pCutoutDepthSkinVS));
		//    SkinCutoutDepthPSO.Finalize();
		//    gPSOs.push_back(SkinCutoutDepthPSO);
		//}


		//ASSERT(gPSOs.size() == 4);

		//// Shadow PSOs
		//{
		//    DepthOnlyPSO.SetRasterizerState(RasterizerShadow);
		//    DepthOnlyPSO.SetRenderTargetFormats(0, nullptr, gShadowBuffer.GetFormat());
		//    DepthOnlyPSO.Finalize();
		//    gPSOs.push_back(DepthOnlyPSO);

		//    CutoutDepthPSO.SetRasterizerState(RasterizerShadowTwoSided);
		//    CutoutDepthPSO.SetRenderTargetFormats(0, nullptr, gShadowBuffer.GetFormat());
		//    CutoutDepthPSO.Finalize();
		//    gPSOs.push_back(CutoutDepthPSO);

		//    SkinDepthOnlyPSO.SetRasterizerState(RasterizerShadow);
		//    SkinDepthOnlyPSO.SetRenderTargetFormats(0, nullptr, gShadowBuffer.GetFormat());
		//    SkinDepthOnlyPSO.Finalize();
		//    gPSOs.push_back(SkinDepthOnlyPSO);

		//    SkinCutoutDepthPSO.SetRasterizerState(RasterizerShadowTwoSided);
		//    SkinCutoutDepthPSO.SetRenderTargetFormats(0, nullptr, gShadowBuffer.GetFormat());
		//    SkinCutoutDepthPSO.Finalize();
		//    gPSOs.push_back(SkinCutoutDepthPSO);

		//    ASSERT(gPSOs.size() == 8);

		//}
		//
		//// Default PSO
		//{
		//    m_DefaultPSO.SetRootSignature(gRootSig);
		//    m_DefaultPSO.SetRasterizerState(RasterizerDefault);
		//    m_DefaultPSO.SetBlendState(BlendDisable);
		//    m_DefaultPSO.SetDepthStencilState(DepthStateReadWrite);
		//    m_DefaultPSO.SetInputLayout(0, nullptr);
		//    m_DefaultPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//    m_DefaultPSO.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
		//    m_DefaultPSO.SetVertexShader(g_pDefaultVS, sizeof(g_pDefaultVS));
		//    m_DefaultPSO.SetPixelShader(g_pDefaultPS, sizeof(g_pDefaultPS));
		//}


		//// Skybox PSO
		//{
		//    m_SkyboxPSO = m_DefaultPSO;
		//    m_SkyboxPSO.SetDepthStencilState(DepthStateReadOnly);
		//    m_SkyboxPSO.SetInputLayout(0, nullptr);
		//    m_SkyboxPSO.SetVertexShader(g_pSkyboxVS, sizeof(g_pSkyboxVS));
		//    m_SkyboxPSO.SetPixelShader(g_pSkyboxPS, sizeof(g_pSkyboxPS));
		//    m_SkyboxPSO.Finalize();
		//}

		gTextureHeap.Create(L"Scene Texture Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);

		//// ミニエンジンのやり方は各テクスチャに対して一つサンプラーに対応
		gSamplerHeap.Create(L"Scene Sampler Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);

		////TODO:lightSystem初期化
		////Lighting::InitializeResources();

		//// 共通テクスチャの記述子テーブルを割り当てる
		//gCommonTextures = gTextureHeap.Alloc(8);

		//uint32_t DestCount = 3;
		//uint32_t SourceCounts[] = { 1, 1, 1};

		//D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[] =
		//{
		//    GetDefaultTexture(kBlackCubeMap),
		//    GetDefaultTexture(kBlackCubeMap),
		//    gShadowBuffer.GetSRV(),
		//    //TODO::LightSystem
		//};

		//DX12Core::gDevice->CopyDescriptors(1, &gCommonTextures, &DestCount, DestCount, SourceTextures, SourceCounts, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//ShadowBufferID = gShadowBuffer.GetVersionID();

		//test
		{
			test = AssetManager::LoadTextureFile(L"Asset/textures/uvChecker.png");
			gpuHandle = gTextureHeap.Alloc();

			DX12Core::gDevice->CopyDescriptorsSimple(
				1,
				gpuHandle,
				test.GetSRV(),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}
	}

	void RenderSystem::Update()
	{
		if (/*SSAOFullScreenID == g_SSAOFullScreen.GetVersionID() &&*/
			ShadowBufferID == gShadowBuffer.GetVersionID())
		{
			return;
		}

		uint32_t DestCount = 1;
		uint32_t SourceCounts[] = { 1 };

		D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[] =
		{
			/*SSAOFullScreenID.GetSRV(),*/
			gShadowBuffer.GetSRV(),
		};

		DescriptorHandle dest = gCommonTextures + gTextureHeap.GetDescriptorSize();

		DX12Core::gDevice->CopyDescriptors(1, &dest, &DestCount, DestCount, SourceTextures, SourceCounts, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//SSAOFullScreenID = g_SSAOFullScreen.GetVersionID();
		ShadowBufferID = gShadowBuffer.GetVersionID();
	}

	void RenderSystem::Render(float deltaTime)
	{

		ImGui::ShowDemoWindow();

		ImGui::Image(gpuHandle.GetGpuPtr(), ImVec2((float)test.Get()->GetWidth(), (float)test.Get()->GetHeight()));
	}

	void RenderSystem::Shutdown()
	{
		skybox.RadianceCubeMap = nullptr;
		skybox.IrradianceCubeMap = nullptr;

		gTextureHeap.Destroy();
		gSamplerHeap.Destroy();
	}

	uint8_t RenderSystem::GetPSO(uint16_t psoFlags)
	{
		//
		//        GraphicsPSO ColorPSO = m_DefaultPSO;
		//
		//        uint16_t Requirements = kHasPosition | kHasNormal;
		//        ASSERT((psoFlags & Requirements) == Requirements);
		//
		//        std::vector<D3D12_INPUT_ELEMENT_DESC> vertexLayout;
		//        if (psoFlags & kHasPosition)
		//            vertexLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT });
		//        if (psoFlags & kHasNormal)
		//            vertexLayout.push_back({ "NORMAL",   0, DXGI_FORMAT_R10G10B10A2_UNORM,  0, D3D12_APPEND_ALIGNED_ELEMENT });
		//        if (psoFlags & kHasTangent)
		//            vertexLayout.push_back({ "TANGENT",  0, DXGI_FORMAT_R10G10B10A2_UNORM,  0, D3D12_APPEND_ALIGNED_ELEMENT });
		//        if (psoFlags & kHasUV0)
		//            vertexLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT });
		//        else
		//            vertexLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT,       1, D3D12_APPEND_ALIGNED_ELEMENT });
		//        if (psoFlags & kHasUV1)
		//            vertexLayout.push_back({ "TEXCOORD", 1, DXGI_FORMAT_R16G16_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT });
		//        if (psoFlags & kHasSkin)
		//        {
		//            vertexLayout.push_back({ "BLENDINDICES", 0, DXGI_FORMAT_R16G16B16A16_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		//            vertexLayout.push_back({ "BLENDWEIGHT", 0, DXGI_FORMAT_R16G16B16A16_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		//        }
		//
		//        ColorPSO.SetInputLayout((uint32_t)vertexLayout.size(), vertexLayout.data());
		//
		//        if (psoFlags & kHasSkin)
		//        {
		//            if (psoFlags & kHasTangent)
		//            {
		//                if (psoFlags & kHasUV1)
		//                {
		//                    ColorPSO.SetVertexShader(g_pDefaultSkinVS, sizeof(g_pDefaultSkinVS));
		//                    ColorPSO.SetPixelShader(g_pDefaultPS, sizeof(g_pDefaultPS));
		//                }
		//                else
		//                {
		//                    ColorPSO.SetVertexShader(g_pDefaultNoUV1SkinVS, sizeof(g_pDefaultNoUV1SkinVS));
		//                    ColorPSO.SetPixelShader(g_pDefaultNoUV1PS, sizeof(g_pDefaultNoUV1PS));
		//                }
		//            }
		//            else
		//            {
		//                if (psoFlags & kHasUV1)
		//                {
		//                    ColorPSO.SetVertexShader(g_pDefaultNoTangentSkinVS, sizeof(g_pDefaultNoTangentSkinVS));
		//                    ColorPSO.SetPixelShader(g_pDefaultNoTangentPS, sizeof(g_pDefaultNoTangentPS));
		//                }
		//                else
		//                {
		//                    ColorPSO.SetVertexShader(g_pDefaultNoTangentNoUV1SkinVS, sizeof(g_pDefaultNoTangentNoUV1SkinVS));
		//                    ColorPSO.SetPixelShader(g_pDefaultNoTangentNoUV1PS, sizeof(g_pDefaultNoTangentNoUV1PS));
		//                }
		//            }
		//        }
		//        else
		//        {
		//            if (psoFlags & kHasTangent)
		//            {
		//                if (psoFlags & kHasUV1)
		//                {
		//                    ColorPSO.SetVertexShader(g_pDefaultVS, sizeof(g_pDefaultVS));
		//                    ColorPSO.SetPixelShader(g_pDefaultPS, sizeof(g_pDefaultPS));
		//                }
		//                else
		//                {
		//                    ColorPSO.SetVertexShader(g_pDefaultNoUV1VS, sizeof(g_pDefaultNoUV1VS));
		//                    ColorPSO.SetPixelShader(g_pDefaultNoUV1PS, sizeof(g_pDefaultNoUV1PS));
		//                }
		//            }
		//            else
		//            {
		//                if (psoFlags & kHasUV1)
		//                {
		//                    ColorPSO.SetVertexShader(g_pDefaultNoTangentVS, sizeof(g_pDefaultNoTangentVS));
		//                    ColorPSO.SetPixelShader(g_pDefaultNoTangentPS, sizeof(g_pDefaultNoTangentPS));
		//                }
		//                else
		//                {
		//                    ColorPSO.SetVertexShader(g_pDefaultNoTangentNoUV1VS, sizeof(g_pDefaultNoTangentNoUV1VS));
		//                    ColorPSO.SetPixelShader(g_pDefaultNoTangentNoUV1PS, sizeof(g_pDefaultNoTangentNoUV1PS));
		//                }
		//            }
		//        }
		//
		//        if (psoFlags & kAlphaBlend)
		//        {
		//            ColorPSO.SetBlendState(BlendPreMultiplied);
		//            ColorPSO.SetDepthStencilState(DepthStateReadOnly);
		//        }
		//        if (psoFlags & kTwoSided)
		//        {
		//            ColorPSO.SetRasterizerState(RasterizerTwoSided);
		//        }
		//        ColorPSO.Finalize();
		//
		//        // 既存のPSOを探す
		//        for (uint32_t i = 0; i < gPSOs.size(); ++i)
		//        {
		//            if (ColorPSO.GetPipelineStateObject() == gPSOs[i].GetPipelineStateObject())
		//            {
		//                return (uint8_t)i;
		//            }
		//        }
		//
		//        //見つからない場合は新しいものを保存し、インデックスを返す
		//        gPSOs.push_back(ColorPSO);
		//
		//        // 返されるPSOインデックスは読み書き可能な深度を持ちます。インデックス+1は、深度が等しいかどうかをテストします。
		//        ColorPSO.SetDepthStencilState(DepthStateTestEqual);
		//        ColorPSO.Finalize();
		//#ifdef _DEBUG
		//        for (uint32_t i = 0; i < sm_PSOs.size(); ++i)
		//            ASSERT(ColorPSO.GetPipelineStateObject() != sm_PSOs[i].GetPipelineStateObject());
		//#endif
		//        gPSOs.push_back(ColorPSO);
		//
		//        ASSERT(gPSOs.size() <= 256, "Ran out of room for unique PSOs");
		//
		return (uint8_t)gPSOs.size() - 2;
	}
	//
	void RenderSystem::SetIBLTextures(TextureRef diffuseIBL, TextureRef specularIBL)
	{
		//		skybox.RadianceCubeMap = specularIBL;
		//		skybox.IrradianceCubeMap = diffuseIBL;
		//
		//		skybox.SpecularIBLRange = 0.0f;
		//		if (skybox.RadianceCubeMap.IsValid())
		//		{
		//			ID3D12Resource* texRes = const_cast<ID3D12Resource*>(skybox.RadianceCubeMap.Get()->GetResource());
		//			const D3D12_RESOURCE_DESC& texDesc = texRes->GetDesc();
		//			skybox.SpecularIBLRange = std::max(0.0f, (float)texDesc.MipLevels - 1);
		//			skybox.SpecularIBLBias = std::min(skybox.SpecularIBLBias, skybox.SpecularIBLRange);
		//		}
		//
		//		uint32_t DestCount = 2;
		//		uint32_t SourceCounts[] = { 1, 1 };
		//
		//		D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[] =
		//		{
		//			specularIBL.IsValid() ? specularIBL.GetSRV() : GetDefaultTexture(kBlackCubeMap),
		//			diffuseIBL.IsValid() ? diffuseIBL.GetSRV() : GetDefaultTexture(kBlackCubeMap)
		//		};
		//
		//		DX12Core::gDevice->CopyDescriptors(1, &gCommonTextures, &DestCount, DestCount, SourceTextures, SourceCounts, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	}

	void RenderSystem::SetIBLBias(float LODBias)
	{
		skybox.SpecularIBLBias = std::min(LODBias, skybox.SpecularIBLRange);
	}
	float RenderSystem::GetIBLRange()
	{
		return skybox.SpecularIBLRange - skybox.SpecularIBLBias;
	}
	float RenderSystem::GetIBLBias()
	{
		return skybox.SpecularIBLBias;
	}
	const PSO& RenderSystem::GetPSO(uint64_t index)
	{
		return gPSOs[index];
	}
}