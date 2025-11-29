#include "ParticleSystem.h"
#include "Runtime/Platform/DirectX12/Buffer/ReadbackBuffer.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Platform/DirectX12/Shader/ShaderCompiler.h"
#include "Runtime/Resource/AssetManager.h"
#include "imgui.h"

namespace AtomEngine
{
	RootSignature ParticleSystem::mParticleSig;
	std::vector<GraphicsPSO> ParticleSystem::mParticlePSOs;
	StructuredBuffer ParticleSystem::sParticleBuffer;

	ComputePSO ParticleSystem::sParticleEmitCS;
	ComputePSO ParticleSystem::sParticleUpdateCS;
	ComputePSO ParticleSystem::sParticleDispatchIndirectArgsCS;
	ComputePSO ParticleSystem::sParticleFinalDispatchIndirectArgsCS;

	IndirectArgsBuffer ParticleSystem::sDrawIndirectArgs;
	IndirectArgsBuffer ParticleSystem::sDispatchIndirectArgs;
	IndirectArgsBuffer ParticleSystem::sFinalDispatchIndirectArgs;

	std::vector<std::unique_ptr<Particle>> ParticleSystem::sParticlePool;
	std::vector<Particle*> ParticleSystem::sParticlesActive;
	BlendMode ParticleSystem::sBlendMode = BlendMode::kBlendAlphaAdd;
	PrewView ParticleSystem::sPrewView;

	bool ParticleSystem::sInitComplete = false;

	struct InputVertex
	{
		Vector3 position;
		Vector2 uv;
	};
	void ParticleSystem::Initialize()
	{
		D3D12_SAMPLER_DESC SamplerBilinearBorderDesc = SamplerPointBorderDesc;
		SamplerBilinearBorderDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;

		mParticleSig.Reset(4, 1);
		mParticleSig.InitStaticSampler(0, SamplerPointBorderDesc);
		mParticleSig[0].InitAsConstantBuffer(0);
		mParticleSig[1].InitAsConstantBuffer(1);
		mParticleSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 5);
		mParticleSig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10);
		mParticleSig.Finalize(L"Particle RootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		D3D12_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		DXGI_FORMAT ColorFormat = gSceneColorBuffer.GetFormat();
		DXGI_FORMAT DepthFormat = gSceneDepthBuffer.GetFormat();

		auto vsBlob = ShaderCompiler::CompileBlob(L"/Particle/ParticleVS.hlsl", L"vs_6_2");
		auto psBlob = ShaderCompiler::CompileBlob(L"/Particle/ParticlePS.hlsl", L"ps_6_2");

		// 修正: DispatchIndirect 引数を書き込むシェーダーは正しいファイルをコンパイルする
		auto particleEmitBlob = ShaderCompiler::CompileBlob(L"/Particle/ParticleEmitCS.hlsl", L"cs_6_2");
		auto particleUpdateBlob = ShaderCompiler::CompileBlob(L"/Particle/ParticleUpdateCS.hlsl", L"cs_6_2");
		// ここを正しいファイルに変更（以前は誤って Final 用を 2 回コンパイルしていた）
		auto particleDispatchIndirectArgs = ShaderCompiler::CompileBlob(L"/Particle/ParticleDispatchIndirectArgs.hlsl", L"cs_6_2");
		auto particleFinalDispatchIndirectArgs = ShaderCompiler::CompileBlob(L"/Particle/ParticleFinalDispatchIndirectArgsCS.hlsli", L"cs_6_2");
		// PSO
		mParticlePSOs.resize(kNumBlendModes);
		for (uint32_t i = 0; i < kNumBlendModes; i++)
		{
			auto& pso = mParticlePSOs[i];

			pso.SetRootSignature(mParticleSig);
			pso.SetRasterizerState(RasterizerTwoSided);
			pso.SetBlendState(gBlendModeTable[BlendMode(i)]);
			pso.SetDepthStencilState(DepthStateReadOnly);
			pso.SetInputLayout(_countof(inputLayout), inputLayout);
			pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
			pso.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
			pso.SetVertexShader(vsBlob.Get());
			pso.SetPixelShader(psBlob.Get());
			pso.Finalize();
		}
		//compute
		sParticleEmitCS.SetRootSignature(mParticleSig);
		sParticleEmitCS.SetComputeShader(particleEmitBlob.Get());
		sParticleEmitCS.Finalize();

		sParticleUpdateCS.SetRootSignature(mParticleSig);
		sParticleUpdateCS.SetComputeShader(particleUpdateBlob.Get());
		sParticleUpdateCS.Finalize();

		sParticleDispatchIndirectArgsCS.SetRootSignature(mParticleSig);
		sParticleDispatchIndirectArgsCS.SetComputeShader(particleDispatchIndirectArgs.Get());
		sParticleDispatchIndirectArgsCS.Finalize();

		sParticleFinalDispatchIndirectArgsCS.SetRootSignature(mParticleSig);
		sParticleFinalDispatchIndirectArgsCS.SetComputeShader(particleFinalDispatchIndirectArgs.Get());
		sParticleFinalDispatchIndirectArgsCS.Finalize();

		sParticleBuffer.Create(L"Particles Buffer", kMaxParticles, sizeof(ParticleVertex));

		__declspec(align(16)) UINT InitialDrawIndirectArgs[4] = { 6, 0, 0, 0 };
		sDrawIndirectArgs.Create(L"ParticleEffectManager DrawIndirectArgs", 1, sizeof(D3D12_DRAW_ARGUMENTS), InitialDrawIndirectArgs);
		__declspec(align(16)) UINT InitialDispatchIndirectArgs[6] = { 0, 1, 1, 0, 1, 1 };
		sFinalDispatchIndirectArgs.Create(L"ParticleEffectManager FinalDispatchIndirectArgs", 1, sizeof(D3D12_DISPATCH_ARGUMENTS), InitialDispatchIndirectArgs);


		sInitComplete = true;
	}

	void ParticleSystem::Update(ComputeContext& Context, float deltaTime)
	{
		if (!sInitComplete || sParticlesActive.size() == 0)return;

		sPrewView.deltaTime = deltaTime;
		sPrewView.time += deltaTime;

		Context.ResetCounter(sParticleBuffer);
		if (sParticlesActive.size() == 0)return;
		Context.SetRootSignature(mParticleSig);
		Context.TransitionResource(sParticleBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.SetDynamicConstantBufferView(0, sizeof(sPrewView), &sPrewView);
		Context.SetDynamicDescriptor(2, 0, sParticleBuffer.GetUAV());

		for (uint32_t i = 0; i < sParticlesActive.size(); ++i)
		{
			sParticlesActive[i]->Update(Context, deltaTime);

			if (sParticlesActive[i]->GetLifetime() <= sParticlesActive[i]->GetElapsedTime())
			{
				//Erase from vector
				auto iter = sParticlesActive.begin() + i;
				static std::mutex s_EraseEffectMutex;
				s_EraseEffectMutex.lock();
				sParticlesActive.erase(iter);
				s_EraseEffectMutex.unlock();
			}
		}

		SetFinalBuffers(Context);
	}

	void ParticleSystem::Render(GraphicsContext& gfxContext, const Camera& camera, ColorBuffer& ColorTarget, DepthBuffer& DepthTarget)
	{
		if (!sInitComplete || sParticlesActive.size() == 0)return;

		sPrewView.ViewProj = camera.GetViewProjMatrix();
		sPrewView.billboardMat = Matrix4x4(
			camera.GetRightVec(),
			camera.GetUpVec(),
			camera.GetForwardVec());

		D3D12_RECT scissor;
		scissor.left = 0;
		scissor.top = 0;
		scissor.right = (LONG)gSceneColorBuffer.GetWidth();
		scissor.bottom = (LONG)gSceneColorBuffer.GetHeight();

		D3D12_VIEWPORT viewport;
		viewport.TopLeftX = 0.0;
		viewport.TopLeftY = 0.0;
		viewport.Width = (float)gSceneColorBuffer.GetWidth();
		viewport.Height = (float)gSceneColorBuffer.GetHeight();
		viewport.MinDepth = 0.0;
		viewport.MaxDepth = 1.0;


		gfxContext.SetRootSignature(mParticleSig);
		gfxContext.SetPipelineState(mParticlePSOs[sBlendMode]);
		gfxContext.TransitionResource(sParticleBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		gfxContext.TransitionResource(sDrawIndirectArgs, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

		static const InputVertex verteices[6] = {
			{{-0.5f,-0.5f,0.0f},{0.0f,1.0f}},
			{{-0.5f,0.5f, 0.0f},{ 0.0f,0.0f}},
			{{0.5f,-0.5f, 0.0f},{ 1.0f,1.0f}},
			{{0.5f,-0.5f, 0.0f},{ 1.0f,1.0f}},
			{{-0.5f,0.5f, 0.0f},{ 0.0f,0.0f}},
			{{0.5f,0.5f,0.0f},{1.0f,0.0f}}
		};
		gfxContext.SetDynamicVB(0, _countof(verteices), sizeof(InputVertex), verteices);

		gfxContext.SetDynamicConstantBufferView(0, sizeof(sPrewView), &sPrewView);
		gfxContext.SetDynamicDescriptor(3, 0, sParticleBuffer.GetSRV());
		for (uint32_t i = 0; i < sParticlesActive.size(); ++i)
		{
			sParticlesActive[i]->Render(gfxContext);
		}

		gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gfxContext.TransitionResource(ColorTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
		gfxContext.TransitionResource(DepthTarget, D3D12_RESOURCE_STATE_DEPTH_READ);
		gfxContext.SetRenderTarget(ColorTarget.GetRTV(), DepthTarget.GetDSV_DepthReadOnly());
		gfxContext.SetViewportAndScissor(viewport, scissor);

		gfxContext.DrawIndirect(sDrawIndirectArgs);
	}

	void ParticleSystem::Shutdown()
	{
		sParticlePool.clear();
		sParticlesActive.clear();
		sParticleBuffer.Destroy();
		sDrawIndirectArgs.Destroy();
		sDispatchIndirectArgs.Destroy();
		sFinalDispatchIndirectArgs.Destroy();
	}

	uint32_t ParticleSystem::CreateParticle(ParticleProperty& props)
	{
		if (!sInitComplete)
			return 0xffffffff;

		static std::mutex s_InstantiateNewEffectMutex;
		s_InstantiateNewEffectMutex.lock();
		Particle* newEffect = new Particle(props);
		sParticlePool.emplace_back(newEffect);
		sParticlesActive.push_back(newEffect);
		s_InstantiateNewEffectMutex.unlock();

		uint32_t index = (uint32_t)sParticlesActive.size() - 1;
		sParticlesActive[index]->Initialize();
		return index;
	}

	void ParticleSystem::ResetParticle(uint32_t particleId)
	{
		if (!sInitComplete || sParticlesActive.size() == 0 || particleId >= sParticlesActive.size())
			return;

		sParticlesActive[particleId]->Reset();
	}

	float ParticleSystem::GetCurrentLife(uint32_t particleId)
	{
		if (!sInitComplete || sParticlesActive.size() == 0 || particleId >= sParticlesActive.size())
			return -1.0;

		return sParticlesActive[particleId]->GetElapsedTime();
	}

	void ParticleSystem::RegisterTexture(uint32_t particleId, const TextureRef& texture)
	{
		if (!sInitComplete || sParticlesActive.size() == 0 || particleId >= sParticlesActive.size())
			return;

        sParticlesActive[particleId]->SetTexture(texture);
	}

	void ParticleSystem::SetFinalBuffers(ComputeContext& CompContext)
	{
		CompContext.SetPipelineState(sParticleFinalDispatchIndirectArgsCS);
		CompContext.TransitionResource(sParticleBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
		CompContext.TransitionResource(sFinalDispatchIndirectArgs, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CompContext.TransitionResource(sDrawIndirectArgs, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CompContext.SetDynamicDescriptor(2, 0, sFinalDispatchIndirectArgs.GetUAV());
		CompContext.SetDynamicDescriptor(2, 1, sDrawIndirectArgs.GetUAV());
		CompContext.SetDynamicDescriptor(3, 0, sParticleBuffer.GetCounterSRV(CompContext));

		CompContext.Dispatch(1, 1, 1);
	}

}