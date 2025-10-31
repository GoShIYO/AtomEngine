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
	using namespace Microsoft::WRL;

	std::vector<GraphicsPSO> Renderer::gPSOs;
	RootSignature Renderer::mRootSig;
	DescriptorHeap Renderer::gTextureHeap;
	DescriptorHandle Renderer::gCommonTextures;

	//uint32_t SSAOFullScreenID;
	uint32_t ShadowBufferID;

	GraphicsPSO m_DefaultPSO(L"Renderer: Default PSO"); // Not finalized.  Used as a template.

	void Renderer::Initialize()
	{
		SamplerDesc DefaultSamplerDesc;
		DefaultSamplerDesc.MaxAnisotropy = 8;

		SamplerDesc CubeMapSamplerDesc = DefaultSamplerDesc;
		//CubeMapSamplerDesc.MaxLOD = 6.0f;

		mRootSig.Reset(kNumRootBindings, 3);

		mRootSig.InitStaticSampler(10, DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		mRootSig.InitStaticSampler(11, SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		mRootSig.InitStaticSampler(12, CubeMapSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);

		mRootSig[kMeshConstants].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
		mRootSig[kMaterialConstants].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);
		mRootSig[kMaterialSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10, D3D12_SHADER_VISIBILITY_PIXEL);
		mRootSig[kCommonSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 10, D3D12_SHADER_VISIBILITY_PIXEL);
		mRootSig[kCommonCBV].InitAsConstantBuffer(1);
		mRootSig[kSkinMatrices].InitAsBufferSRV(20, D3D12_SHADER_VISIBILITY_VERTEX);
		mRootSig.Finalize(L"RootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		DXGI_FORMAT ColorFormat = gSceneColorBuffer.GetFormat();
		DXGI_FORMAT DepthFormat = gSceneDepthBuffer.GetFormat();

		D3D12_INPUT_ELEMENT_DESC defaultInput[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{ "BITTANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		D3D12_INPUT_ELEMENT_DESC skinInput[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	 0},
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	 0},
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	 0},
			{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	 0},
			{ "BITTANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	 0},
			{ "WEIGHT",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{ "INDEX ",   0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};

		//default PSO
		auto defaultVS = ShaderCompiler::CompileBlob(L"Object3D.VS.hlsl", L"vs_6_2");
		auto defaultPS = ShaderCompiler::CompileBlob(L"Object3D.PS.hlsl", L"ps_6_2");

		GraphicsPSO defaultPSO(L"default PSO");
		defaultPSO.SetRootSignature(mRootSig);
		defaultPSO.SetRasterizerState(RasterizerDefaultCw);
		defaultPSO.SetBlendState(BlendDisable);
		defaultPSO.SetDepthStencilState(DepthStateReadWrite);
		defaultPSO.SetInputLayout(_countof(defaultInput), defaultInput);
		defaultPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		defaultPSO.SetRenderTargetFormat(ColorFormat, DepthFormat);
		defaultPSO.SetVertexShader(defaultVS.Get());
		defaultPSO.SetPixelShader(defaultPS.Get());
		defaultPSO.Finalize();
		gPSOs.push_back(defaultPSO);

		////skin PSO
		//auto skinVS = ShaderCompiler::CompileBlob(L"Object3DSkin.VS.hlsl", L"vs_6_2");
		//GraphicsPSO skinPSO(L"skin PSO");
		//skinPSO = defaultPSO;
		//skinPSO.SetInputLayout(_countof(skinInput), skinInput);
		//skinPSO.SetVertexShader(skinVS.Get());
		//skinPSO.Finalize();
		//gPSOs.push_back(skinPSO);

		////MetallicRoughness Workflow PSO
		//auto mrWorkflowPS = ShaderCompiler::CompileBlob(L"Object3DMRWorkflow.PS.hlsl", L"ps_6_2");
		//GraphicsPSO mrWorkflowPSO(L"MR workflow PSO");
		//mrWorkflowPSO = defaultPSO;
  //      mrWorkflowPSO.SetPixelShader(mrWorkflowPS.Get());
		//mrWorkflowPSO.Finalize();
		//gPSOs.push_back(mrWorkflowPSO);

		////MetallicRoughness Workflow Skin PSO
		//GraphicsPSO mrWorkflowSkinPSO(L"MR workflow skin PSO");
		//mrWorkflowSkinPSO = mrWorkflowPSO;
		//mrWorkflowSkinPSO.SetVertexShader(skinVS.Get());
		//mrWorkflowSkinPSO.Finalize();
		//gPSOs.push_back(mrWorkflowSkinPSO);



		gTextureHeap.Create(L"Scene Texture Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);

		//TODO:lightSystem初期化
		//Lighting::InitializeResources();

		uint32_t DestCount = 3;
		uint32_t SourceCounts[] = { 1, 1, 1 };

		//// 共通テクスチャの記述子テーブルを割り当てる
		gCommonTextures = gTextureHeap.Alloc(3);

		D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[] =
		{
			GetDefaultTexture(kBlackCubeMap),
			GetDefaultTexture(kBlackCubeMap),
			gShadowBuffer.GetSRV(),
			//TODO::LightSystem
		};

		DX12Core::gDevice->CopyDescriptors(1, &gCommonTextures,
			&DestCount, DestCount, SourceTextures, SourceCounts,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//ShadowBufferID = gShadowBuffer.GetVersionID();
	}

	void Renderer::Update(float deltaTime)
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

	void Renderer::Render(float deltaTime)
	{


	}

	void Renderer::Shutdown()
	{
		gTextureHeap.Destroy();
	}
}