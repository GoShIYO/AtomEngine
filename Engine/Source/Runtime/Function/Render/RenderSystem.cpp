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
#include "Runtime/Function/Light/LightManager.h"
#include "Runtime/Resource/Model.h"
#include "Runtime/Core/Utility/Utility.h"
#include "Runtime/Function/Framework/Component/MeshComponent.h"
#include "Runtime/Function/Render/RenderPasses/RenderPassManager.h"
#include "Runtime/Function/Render/RenderPasses/RenderPassesInclude.h"

namespace AtomEngine
{
	using namespace Microsoft::WRL;

	std::vector<GraphicsPSO> Renderer::gPSOs;
	RootSignature Renderer::mRootSig;
	DescriptorHeap Renderer::gTextureHeap;
	DescriptorHandle Renderer::gCommonTextures;

	uint32_t SSAOFullScreenID;
	uint32_t ShadowBufferID;

	D3D12_VIEWPORT mViewport{};
	D3D12_RECT mScissor{};
	RenderPassManager* mRenderPassManager = nullptr;

	void Renderer::Initialize()
	{
		SamplerDesc DefaultSamplerDesc;
		DefaultSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		mRootSig.Reset(kNumRootBindings, 3);

		mRootSig.InitStaticSampler(10, DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		mRootSig.InitStaticSampler(11, SamplerAnisoWrapDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		mRootSig.InitStaticSampler(12, SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);

		mRootSig[kMeshConstants].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
		mRootSig[kMaterialConstants].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);
		mRootSig[kMaterialSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10, D3D12_SHADER_VISIBILITY_PIXEL);
		mRootSig[kCommonSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 10, D3D12_SHADER_VISIBILITY_PIXEL);
		mRootSig[kCommonCBV].InitAsConstantBuffer(1);
        mRootSig[kLightConstants].InitAsConstantBuffer(2, D3D12_SHADER_VISIBILITY_PIXEL);
		mRootSig[kSkinMatrices].InitAsBufferSRV(20, D3D12_SHADER_VISIBILITY_VERTEX);
		mRootSig.Finalize(L"RootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		DXGI_FORMAT ColorFormat = gSceneColorBuffer.GetFormat();
		DXGI_FORMAT DepthFormat = gSceneDepthBuffer.GetFormat();

		D3D12_INPUT_ELEMENT_DESC posOnlyInput[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_INPUT_ELEMENT_DESC posOnlySkinInput[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "WEIGHT",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{ "INDEX",   0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};

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
			{ "INDEX",   0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
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
		defaultPSO.SetRenderTargetFormats(1,&ColorFormat, DepthFormat);
		defaultPSO.SetVertexShader(defaultVS.Get());
		defaultPSO.SetPixelShader(defaultPS.Get());
		defaultPSO.Finalize();
		gPSOs.push_back(defaultPSO);

		//skin PSO
		auto skinVS = ShaderCompiler::CompileBlob(L"Object3DSkinned.VS.hlsl", L"vs_6_2");
		GraphicsPSO skinPSO(L"skin PSO");
		skinPSO = defaultPSO;
		skinPSO.SetInputLayout(_countof(skinInput), skinInput);
		skinPSO.SetVertexShader(skinVS.Get());
		skinPSO.Finalize();
		gPSOs.push_back(skinPSO);

		//MetallicRoughness Workflow PSO
		auto mrWorkflowPS = ShaderCompiler::CompileBlob(L"Object3DMR.PS.hlsl", L"ps_6_2");
		GraphicsPSO mrWorkflowPSO(L"MR workflow PSO");
		mrWorkflowPSO = defaultPSO;
        mrWorkflowPSO.SetPixelShader(mrWorkflowPS.Get());
		mrWorkflowPSO.Finalize();
		gPSOs.push_back(mrWorkflowPSO);

		//MetallicRoughness Workflow Skin PSO
		GraphicsPSO mrWorkflowSkinPSO(L"MR workflow skin PSO");
		mrWorkflowSkinPSO = mrWorkflowPSO;
		mrWorkflowSkinPSO.SetInputLayout(_countof(skinInput), skinInput);
		mrWorkflowSkinPSO.SetVertexShader(skinVS.Get());
		mrWorkflowSkinPSO.Finalize();
		gPSOs.push_back(mrWorkflowSkinPSO);

		//Transparent PSO
		GraphicsPSO transparentPSO(L"transparent PSO");
		transparentPSO = defaultPSO;
		transparentPSO.SetBlendState(BlendPreMultiplied);
		transparentPSO.SetDepthStencilState(DepthStateReadOnly);
		transparentPSO.Finalize();
		gPSOs.push_back(transparentPSO);

		//Transparent Skin PSO
		GraphicsPSO transparentSkinPSO(L"transparent skin PSO");
		transparentSkinPSO = skinPSO;
		transparentSkinPSO.SetBlendState(BlendPreMultiplied);
		transparentSkinPSO.SetDepthStencilState(DepthStateReadOnly);
		transparentSkinPSO.Finalize();
		gPSOs.push_back(transparentSkinPSO);

		//Transparent MR Workflow PSO
		GraphicsPSO transparentMRWorkflowPSO(L"transparent MR workflow PSO");
		transparentMRWorkflowPSO = mrWorkflowPSO;
		transparentMRWorkflowPSO.SetBlendState(BlendPreMultiplied);
		transparentMRWorkflowPSO.SetDepthStencilState(DepthStateReadOnly);
		transparentMRWorkflowPSO.Finalize();
		gPSOs.push_back(transparentMRWorkflowPSO);

		//Transparent MR Workflow Skin PSO
		GraphicsPSO transparentMRWorkflowSkinPSO(L"transparent MR workflow skin PSO");
		transparentMRWorkflowSkinPSO = mrWorkflowSkinPSO;
		transparentMRWorkflowSkinPSO.SetBlendState(BlendPreMultiplied);
		transparentMRWorkflowSkinPSO.SetDepthStencilState(DepthStateReadOnly);
		transparentMRWorkflowSkinPSO.Finalize();
		gPSOs.push_back(transparentMRWorkflowSkinPSO);

		//Depth Only PSO
		auto depthOnlyVS = ShaderCompiler::CompileBlob(L"DepthOnlyVS.hlsl", L"vs_6_2");
		GraphicsPSO depthOnlyPSO(L"depth only PSO");
		depthOnlyPSO.SetRootSignature(mRootSig);
		depthOnlyPSO.SetRasterizerState(RasterizerDefaultCw);
		depthOnlyPSO.SetBlendState(BlendDisable);
		depthOnlyPSO.SetDepthStencilState(DepthStateReadWrite);
		depthOnlyPSO.SetInputLayout(_countof(posOnlyInput), posOnlyInput);
		depthOnlyPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		depthOnlyPSO.SetRenderTargetFormats(0, nullptr, DepthFormat);
		depthOnlyPSO.SetVertexShader(depthOnlyVS.Get());
		depthOnlyPSO.Finalize();
		gPSOs.push_back(depthOnlyPSO);

		//Depth Only Skin PSO
		auto shadowSkinVS = ShaderCompiler::CompileBlob(L"DepthOnlySkinVS.hlsl", L"vs_6_2");
		GraphicsPSO SkinDepthOnlyPSO(L"depthOnly Skin PSO");
		SkinDepthOnlyPSO = depthOnlyPSO;
		SkinDepthOnlyPSO.SetInputLayout(_countof(posOnlySkinInput), posOnlySkinInput);
		SkinDepthOnlyPSO.SetVertexShader(shadowSkinVS.Get());
		SkinDepthOnlyPSO.Finalize();
		gPSOs.push_back(SkinDepthOnlyPSO);

		//shadow PSO
		GraphicsPSO shadowPSO = depthOnlyPSO;
		shadowPSO.SetRasterizerState(RasterizerShadow);
		shadowPSO.SetRenderTargetFormats(0,nullptr, gShadowBuffer.GetFormat());
		shadowPSO.Finalize();
		gPSOs.push_back(shadowPSO);

		//Shadow Skin PSO
		GraphicsPSO shadowSkinPSO = SkinDepthOnlyPSO;
		shadowSkinPSO.SetRasterizerState(RasterizerShadow);
		shadowSkinPSO.SetRenderTargetFormats(0, nullptr, gShadowBuffer.GetFormat());
		shadowSkinPSO.Finalize();
		gPSOs.push_back(shadowSkinPSO);

		gTextureHeap.Create(L"Scene Texture Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);

		//lightSystem初期化
		LightManager::Initialize();

		uint32_t DestCount = 9;
		std::vector<uint32_t>SourceCounts(DestCount, 1);

		//// 共通テクスチャの記述子テーブルを割り当てる
		gCommonTextures = gTextureHeap.Alloc(9);

		D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[] =
		{
			GetDefaultTexture(kBlackCubeMap),
			GetDefaultTexture(kBlackCubeMap),
			GetDefaultTexture(kDefaultBRDFLUT),
			gShadowBuffer.GetSRV(),
			gSSAOFullScreen.GetSRV(),
			LightManager::GetLightSrv(),
			LightManager::GetLightShadowSrv(),
			LightManager::GetLightGridSrv(),
			LightManager::GetLightGridBitMaskSrv()
		};

		DX12Core::gDevice->CopyDescriptors(1, &gCommonTextures,
			&DestCount, DestCount, SourceTextures, SourceCounts.data(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		SSAOFullScreenID = gSSAOFullScreen.GetVersionID();
		ShadowBufferID = gShadowBuffer.GetVersionID();
		InitializeRenderPasses();
	}

	void Renderer::InitializeRenderPasses()
	{
		mRenderPassManager = RenderPassManager::GetInstance();
		mRenderPassManager->RegisterRenderPass(std::make_unique<ForwardRenderingPass>());


		mRenderPassManager->SetUpRenderPass();
	}

	void Renderer::Update(World& world,float deltaTime)
	{
		GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Update");
		//ワールドからメッシュコンポーネントとトランスフォームコンポーネントを取得
		auto view = world.View<MeshComponent,TransformComponent,MaterialComponent>();

		for (auto entity : view)
		{
			//メッシュコンポーネントを更新
			auto& mesh = view.get<MeshComponent>(entity);
			auto& material = view.get<MaterialComponent>(entity);
			auto& transform = view.get<TransformComponent>(entity);
            mesh.Update(gfxContext,transform, material,deltaTime);
		}
		gfxContext.Finish();

		mViewport.Width = (float)gSceneColorBuffer.GetWidth();
		mViewport.Height = (float)gSceneColorBuffer.GetHeight();
		mViewport.MinDepth = 0.0f;
		mViewport.MaxDepth = 1.0f;

		mScissor.left = 0;
		mScissor.top = 0;
		mScissor.right = (LONG)gSceneColorBuffer.GetWidth();
		mScissor.bottom = (LONG)gSceneColorBuffer.GetHeight();

		UpdateBuffers();
	}

	void Renderer::UpdateBuffers()
	{
		if (SSAOFullScreenID == gSSAOFullScreen.GetVersionID() &&
			ShadowBufferID == gShadowBuffer.GetVersionID())
		{
			return;
		}

		uint32_t DestCount = 1;
		std::vector<uint32_t>SourceCounts(DestCount, 1);

		D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[] =
		{
			gSSAOFullScreen.GetSRV(),
			gShadowBuffer.GetSRV(),
		};

		DescriptorHandle dest = gCommonTextures + gTextureHeap.GetDescriptorSize();

		DX12Core::gDevice->CopyDescriptors(1, &dest, &DestCount,
			DestCount, SourceTextures, SourceCounts.data(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


		SSAOFullScreenID = gSSAOFullScreen.GetVersionID();
		ShadowBufferID = gShadowBuffer.GetVersionID();
	}

	void Renderer::Render(World& world, Camera& camera,float deltaTime)
	{
		GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");

		mRenderPassManager->SetWorld(world);
		mRenderPassManager->SetCamera(&camera);
		mRenderPassManager->SetRenderTarget(&gSceneColorBuffer, &gSceneDepthBuffer);
		mRenderPassManager->SetViewportAndScissor(mViewport, mScissor);
		mRenderPassManager->RenderAll(gfxContext);

		gfxContext.Finish();
	}

	void Renderer::Shutdown()
	{
		gTextureHeap.Destroy();
		mRenderPassManager->Shutdown();
		LightManager::Shutdown();
	}

	const GraphicsPSO& Renderer::GetPSO(uint16_t psoFlags)
	{
		return gPSOs[psoFlags];
	}
}