#include "RenderSystem.h"

#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"
#include "Runtime/Platform/DirectX12/Pipline/PiplineState.h"
#include "Runtime/Platform/DirectX12/Buffer/ColorBuffer.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Platform/DirectX12/Shader/ShaderCompiler.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"
#include "Runtime/Function/Render/RenderQueue.h"
#include "Runtime/Resource/AssetManager.h"

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

	using namespace Microsoft::WRL;
	std::unordered_map<std::string,ComPtr<IDxcBlob>> Shaders;

	void RenderSystem::Initialize()
	{
		SamplerDesc DefaultSamplerDesc;
		DefaultSamplerDesc.MaxAnisotropy = 8;

		SamplerDesc CubeMapSamplerDesc = DefaultSamplerDesc;
		//CubeMapSamplerDesc.MaxLOD = 6.0f;

		gRootSig.Reset(kNumRootBindings, 3);

		gRootSig.InitStaticSampler(10, DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		gRootSig.InitStaticSampler(11, SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		gRootSig.InitStaticSampler(12, CubeMapSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);

		gRootSig[kMeshConstants].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
		gRootSig[kMaterialConstants].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);
		gRootSig[kMaterialSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10, D3D12_SHADER_VISIBILITY_PIXEL);
		gRootSig[kCommonSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 10, D3D12_SHADER_VISIBILITY_PIXEL);
		gRootSig[kCommonCBV].InitAsConstantBuffer(1);
		gRootSig[kSkinMatrices].InitAsBufferSRV(20, D3D12_SHADER_VISIBILITY_VERTEX);

		gRootSig.Finalize(L"RootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		gTextureHeap.Create(L"Scene Texture Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);

		//TODO:lightSystem初期化
		//Lighting::InitializeResources();

		//uint32_t DestCount = 3;
		//uint32_t SourceCounts[] = { 1, 1, 1 };

		//D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[] =
		//{
		//	GetDefaultTexture(kBlackCubeMap),
		//	GetDefaultTexture(kBlackCubeMap),
		//	gShadowBuffer.GetSRV(),
		//	//TODO::LightSystem
		//};

		//// 共通テクスチャの記述子テーブルを割り当てる
		//gCommonTextures = gTextureHeap.Alloc(_countof(SourceTextures));

		//DX12Core::gDevice->CopyDescriptors(1, &gCommonTextures, &DestCount, DestCount, SourceTextures, SourceCounts, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//ShadowBufferID = gShadowBuffer.GetVersionID();

		//test
		{
			gpuHandle = gTextureHeap.Alloc();

			test = AssetManager::LoadCovertTexture(L"Asset/textures/uvChecker.png");
			test.CopyGPU(gTextureHeap.GetHeapPointer(), gpuHandle);

		}

	}

	void RenderSystem::Update(float deltaTime)
	{
		//if (/*SSAOFullScreenID == g_SSAOFullScreen.GetVersionID() &&*/
		//	ShadowBufferID == gShadowBuffer.GetVersionID())
		//{
		//	return;
		//}

		//uint32_t DestCount = 1;
		//uint32_t SourceCounts[] = { 1 };

		//D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[] =
		//{
		//	/*SSAOFullScreenID.GetSRV(),*/
		//	gShadowBuffer.GetSRV(),
		//};

		//DescriptorHandle dest = gCommonTextures + gTextureHeap.GetDescriptorSize();

		//DX12Core::gDevice->CopyDescriptors(1, &dest, &DestCount, DestCount, SourceTextures, SourceCounts, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


		////SSAOFullScreenID = g_SSAOFullScreen.GetVersionID();
		//ShadowBufferID = gShadowBuffer.GetVersionID();

	}

	void RenderSystem::Render(float deltaTime)
	{

		ImGui::ShowDemoWindow();
		ImGui::ShowMetricsWindow();
		ImGui::Image((ImTextureID)gpuHandle.GetGpuPtr(), ImVec2((float)test.Get()->GetWidth(), (float)test.Get()->GetHeight()));

	}

	void RenderSystem::Shutdown()
	{
		skybox.RadianceCubeMap = nullptr;
		skybox.IrradianceCubeMap = nullptr;

		gTextureHeap.Destroy();
	}

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
}