#include "ParticleSystem.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Platform/DirectX12/Shader/ShaderCompiler.h"
#include "Runtime/Resource/AssetManager.h"
#include "imgui.h"

namespace AtomEngine
{
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
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		DXGI_FORMAT ColorFormat = gSceneColorBuffer.GetFormat();
		DXGI_FORMAT DepthFormat = gSceneDepthBuffer.GetFormat();

		auto vsBlob = ShaderCompiler::CompileBlob(L"/Particle/ParticleVS.hlsl", L"vs_6_2");
		auto psBlob = ShaderCompiler::CompileBlob(L"/Particle/ParticlePS.hlsl", L"ps_6_2");

		auto particleInitBlob = ShaderCompiler::CompileBlob(L"/Particle/ParticleInitCS.hlsl", L"cs_6_2");
		auto particleUpdateBlob = ShaderCompiler::CompileBlob(L"/Particle/ParticleUpdateCS.hlsl", L"cs_6_2");
		auto emitterBlob = ShaderCompiler::CompileBlob(L"/Particle/EmitterCS.hlsl", L"cs_6_2");

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
		mParticleInitCS.SetRootSignature(mParticleSig);
		mParticleInitCS.SetComputeShader(particleInitBlob.Get());
		mParticleInitCS.Finalize();

		mParticleUpdateCS.SetRootSignature(mParticleSig);
		mParticleUpdateCS.SetComputeShader(particleUpdateBlob.Get());
		mParticleUpdateCS.Finalize();

		mEmitterCS.SetRootSignature(mParticleSig);
		mEmitterCS.SetComputeShader(emitterBlob.Get());
        mEmitterCS.Finalize();


		//mParticleFreeListCS.SetRootSignature(mParticleSig);

		//vertex
		mVerteices.resize(6);
		mVerteices[0] = { {-0.5f,-0.5f,0.0f,1.0f} ,{0.0f,1.0f} };
		mVerteices[1] = { {-0.5f,0.5f, 0.0f,1.0f},{ 0.0f,0.0f } };
		mVerteices[2] = { {0.5f,-0.5f, 0.0f,1.0f},{ 1.0f,1.0f } };
		mVerteices[3] = mVerteices[2];
		mVerteices[4] = mVerteices[1];
		mVerteices[5] = { {0.5f,0.5f,0.0f,1.0f},{1.0f,0.0f} };

		mParticleBuffer.Create(L"Particles Buffer", kMaxParticles, sizeof(Particle));
		mParticleFreeList.Create(L"Particles FreeList", kMaxParticles,sizeof(uint32_t));
		mParticleFreeListIndex.Create(L"Particles FreeListIndex", kMaxParticles, sizeof(uint32_t));

		__declspec(align(16)) UINT InitialDrawIndirectArgs[4] = { 6, 0, 0, 0 };
		mDrawIndirectArgs.Create(L"Particles DrawIndirectArgs", 1, sizeof(D3D12_DRAW_ARGUMENTS), InitialDrawIndirectArgs);
		
		tex = AssetManager::LoadCovertTexture("Asset/Textures/circle2.png");

		// Init particles
		ComputeContext& context = ComputeContext::Begin(L"Particle Init");
		context.TransitionResource(mParticleBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		context.TransitionResource(mParticleFreeList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		context.TransitionResource(mParticleFreeListIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		context.SetRootSignature(mParticleSig);
		context.SetPipelineState(mParticleInitCS);
		context.SetDynamicDescriptor(2, 0, mParticleBuffer.GetUAV());
		context.SetDynamicDescriptor(2, 1, mParticleFreeList.GetUAV());
		context.SetDynamicDescriptor(2, 2, mParticleFreeListIndex.GetUAV());
		context.Dispatch((kMaxParticles + 255) / 256, 1, 1);
		context.Finish(true);

		// Setup emitter
		mEmitter.translate = Vector3(0, 0, 0);
		mEmitter.radius = 10.0f;
		mEmitter.count = kMaxParticles;
		mEmitter.frequency = 0.1f;
		mEmitter.frequencyTime = 0.0f;
		mEmitter.emit = 0;
	}

	void ParticleSystem::Update(ComputeContext& Context,float deltaTime)
	{
		mPrewView.deltaTime = deltaTime;
		mPrewView.time += deltaTime;

		mEmitter.frequencyTime += deltaTime;
		if (mEmitter.frequency <= mEmitter.frequencyTime)
		{
			mEmitter.frequencyTime -= mEmitter.frequency;
			mEmitter.emit = 1;
		}
		else
		{
			mEmitter.emit = 0;
		}

		ImGui::Begin("Particle");
        ImGui::SliderFloat("Particle Radius", &mEmitter.radius, 0.0f, 100.0f);
        ImGui::SliderInt("Particle Count", (int*)&mEmitter.count, 0, kMaxParticles);
		ImGui::SliderFloat("Particle Frequency", &mEmitter.frequency, 0.0f, 1.0f);
		ImGui::DragFloat3("Particle Position", &mEmitter.translate.x,0.01f);

		const char* blendModeNames[] = { "None", "Normal", "AlphaAdd", "Screen" };
		int currentBlendMode = static_cast<int>(mBlendMode);
		if (ImGui::Combo("Blend Mode", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames)))
		{
			mBlendMode = static_cast<BlendMode>(currentBlendMode);
		}
		ImGui::End();

		Context.TransitionResource(mParticleBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(mParticleFreeList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(mParticleFreeListIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		//emitter
		Context.SetRootSignature(mParticleSig);
		Context.SetPipelineState(mEmitterCS);
		Context.SetDynamicConstantBufferView(0, sizeof(mPrewView), &mPrewView);
		Context.SetDynamicConstantBufferView(1, sizeof(mEmitter), &mEmitter);
		Context.SetDynamicDescriptor(2, 0, mParticleBuffer.GetUAV());
		Context.SetDynamicDescriptor(2, 1, mParticleFreeList.GetUAV());
		Context.SetDynamicDescriptor(2, 2, mParticleFreeListIndex.GetUAV());
		Context.Dispatch(1, 1, 1);

		//update
		Context.SetRootSignature(mParticleSig);
		Context.SetPipelineState(mParticleUpdateCS);
		Context.SetDynamicConstantBufferView(0, sizeof(mPrewView), &mPrewView);
		Context.SetDynamicConstantBufferView(1, sizeof(mEmitter), &mEmitter);
		Context.SetDynamicDescriptor(2, 0, mParticleBuffer.GetUAV());
		Context.SetDynamicDescriptor(2, 1, mParticleFreeList.GetUAV());
		Context.SetDynamicDescriptor(2, 2, mParticleFreeListIndex.GetUAV());
		Context.Dispatch((mEmitter.count + 255) / 256, 1, 1);
	}

	void ParticleSystem::Render(GraphicsContext& gfxContext, const Camera& camera)
	{
		mPrewView.ViewProj = camera.GetViewProjMatrix();
		mPrewView.billboardMat = Matrix4x4(
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

		gfxContext.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
		gfxContext.TransitionResource(mParticleBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		gfxContext.SetRootSignature(mParticleSig);
		gfxContext.SetPipelineState(mParticlePSOs[mBlendMode]);

		gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gfxContext.SetRenderTarget(gSceneColorBuffer.GetRTV(), gSceneDepthBuffer.GetDSV_ReadOnly());
		gfxContext.SetViewportAndScissor(viewport, scissor);
		
		gfxContext.SetDynamicVB(0, mVerteices.size(), sizeof(ParticleVertex), mVerteices.data());

		gfxContext.SetDynamicConstantBufferView(0, sizeof(mPrewView), &mPrewView);
		gfxContext.SetDynamicConstantBufferView(1, sizeof(mEmitter), &mEmitter);
		gfxContext.SetDynamicDescriptor(3, 0, mParticleBuffer.GetSRV());
		gfxContext.SetDynamicDescriptor(3, 1, tex.GetSRV());

		gfxContext.DrawInstanced((UINT)mVerteices.size(), mEmitter.count);
	}

	void ParticleSystem::Shutdown()
	{
		mVerteices.clear();
		mParticlePSOs.clear();
		mParticleBuffer.Destroy();
	}

}