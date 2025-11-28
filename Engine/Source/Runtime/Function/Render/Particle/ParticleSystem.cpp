#include "ParticleSystem.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Platform/DirectX12/Shader/ShaderCompiler.h"
#include "Runtime/Resource/AssetManager.h"
#include "imgui.h"
#include "ParticleConfig.h"
#include <cmath>

namespace AtomEngine
{
	void ParticleSystem::LoadPresetsFromJson(const std::string& relativePath)
	{
		mPresets = LoadParticlePresetsFromJson(relativePath);
		if (!mPresets.empty())
		{
			mSelectedPresetIndex = 0;
			auto& p = mPresets[0];
			mBlendMode = p.blend;
			tex = AssetManager::LoadCovertTexture(p.texturePath);
			mEmitter.radius = p.radius;
			mEmitter.count = p.count;
			mEmitter.frequency = p.frequency;
			mEmitter.translate = p.position;

			// 色の設定
			mUpdate.colorStart = Vector4(p.colorStart.x, p.colorStart.y, p.colorStart.z, p.colorStart.w);
			mUpdate.colorEnd   = Vector4(p.colorEnd.x, p.colorEnd.y, p.colorEnd.z, p.colorEnd.w);
		}
	}

	void ParticleSystem::Initialize()
	{
		D3D12_SAMPLER_DESC SamplerBilinearBorderDesc = SamplerPointBorderDesc;
		SamplerBilinearBorderDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;

		mParticleSig.Reset(5, 1);
		mParticleSig.InitStaticSampler(0, SamplerPointBorderDesc);
		mParticleSig[0].InitAsConstantBuffer(0);
		mParticleSig[1].InitAsConstantBuffer(1);
		mParticleSig[2].InitAsConstantBuffer(2);
		mParticleSig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 5);
		mParticleSig[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10);
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


		//vertex
		mVerteices.resize(6);
		mVerteices[0] = { {-0.5f,-0.5f,0.0f,1.0f} ,{0.0f,1.0f} };
		mVerteices[1] = { {-0.5f,0.5f, 0.0f,1.0f},{ 0.0f,0.0f } };
		mVerteices[2] = { {0.5f,-0.5f, 0.0f,1.0f},{ 1.0f,1.0f } };
		mVerteices[3] = mVerteices[2];
		mVerteices[4] = mVerteices[1];
		mVerteices[5] = { {0.5f,0.5f,0.0f,1.0f},{1.0f,0.0f} };

		mParticleBuffer.Create(L"Particles Buffer", kMaxParticles, sizeof(Particle));
		mParticleFreeList.Create(L"Particles FreeList", kMaxParticles, sizeof(uint32_t));
		mParticleFreeListIndex.Create(L"Particles FreeListIndex", kMaxParticles, sizeof(uint32_t));

		__declspec(align(16)) UINT InitialDrawIndirectArgs[4] = { 6, 0, 0, 0 };
		mDrawIndirectArgs.Create(L"Particles DrawIndirectArgs", 1, sizeof(D3D12_DRAW_ARGUMENTS), InitialDrawIndirectArgs);

		// デフォルトテクスチャ
		tex = AssetManager::LoadCovertTexture("Asset/Textures/circle2.png");

		// JSON からプリセットをロード（存在すれば適用）
		LoadPresetsFromJson("Asset/Particles/particles.json");

		// Init particles
		ComputeContext& context = ComputeContext::Begin(L"Particle Init");
		context.TransitionResource(mParticleBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		context.TransitionResource(mParticleFreeList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		context.TransitionResource(mParticleFreeListIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		context.SetRootSignature(mParticleSig);
		context.SetPipelineState(mParticleInitCS);
		context.SetDynamicDescriptor(3, 0, mParticleBuffer.GetUAV());
		context.SetDynamicDescriptor(3, 1, mParticleFreeList.GetUAV());
		context.SetDynamicDescriptor(3, 2, mParticleFreeListIndex.GetUAV());
		context.Dispatch((kMaxParticles + 255) / 256, 1, 1);
		context.Finish(true);

		// Setup emitter (デフォルト or JSON で上書き済み)
		if (mEmitter.count == 0)
		{
			mEmitter.translate = Vector3(0, 0, 0);
			mEmitter.radius = 10.0f;
			mEmitter.count = kMaxParticles;
			mEmitter.frequency = 0.1f;
			mEmitter.frequencyTime = 0.0f;
			mEmitter.emit = 0;
		}

		mPrewView.time = 0.0f;
		mUpdate.colorStart = Vector4(1,1,1,1);
		mUpdate.colorEnd   = Vector4(1,1,1,1);
	}

	void ParticleSystem::Update(ComputeContext& Context, float deltaTime)
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

		static bool openAdvanced = false;
		static char texturePathBuf[260] = "";

		ImGui::Begin("Particle");

		// --- Presets & Texture ---
		if (!mPresets.empty())
		{
			std::vector<const char*> presetNames;
			presetNames.reserve(mPresets.size());
			for (auto& p : mPresets) presetNames.push_back(p.name.c_str());

			int sel = mSelectedPresetIndex < 0 ? 0 : mSelectedPresetIndex;
			if (ImGui::Combo("Presets", &sel, presetNames.data(), (int)presetNames.size()))
			{
				mSelectedPresetIndex = sel;
				auto& s = mPresets[sel];
				mBlendMode = s.blend;
				tex = AssetManager::LoadCovertTexture(s.texturePath);
				mEmitter.radius = s.radius;
				mEmitter.count = s.count;
				mEmitter.frequency = s.frequency;
				mEmitter.translate = s.position;
			}
			ImGui::SameLine();
			if (ImGui::Button("Reload Presets")) LoadPresetsFromJson("Asset/Particles/particles.json");
		}

		// Texture path input
		ImGui::InputText("Texture Path", texturePathBuf, sizeof(texturePathBuf));
		ImGui::SameLine();
		if (ImGui::Button("Load Texture"))
		{
			std::string p = texturePathBuf;
			if (!p.empty())
			{
				auto newTex = AssetManager::LoadCovertTexture(p);
				if (newTex.IsValid())
					tex = newTex;
			}
		}
		ImGui::Separator();

		// --- Emitter ---
		if (ImGui::CollapsingHeader("Emitter", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat3("Position", &mEmitter.translate.x, 0.01f);
			ImGui::SliderFloat("Radius", &mEmitter.radius, 0.0f, 100.0f);
			int count = (int)mEmitter.count;
			if (ImGui::InputInt("Count", &count))
			{
				if (count < 0) count = 0;
				if (count > (int)kMaxParticles) count = kMaxParticles;
				mEmitter.count = (uint32_t)count;
			}
			ImGui::SliderFloat("Frequency", &mEmitter.frequency, 0.0f, 1.0f);
			bool emitFlag = mEmitter.emit != 0;
			if (ImGui::Checkbox("Emit (manual)", &emitFlag))
			{
				mEmitter.emit = emitFlag ? 1 : 0;
			}
			ImGui::SameLine();
			if (ImGui::Button("Burst"))
			{
				mEmitter.emit = 1;
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop"))
			{
				mEmitter.emit = 0;
			}
			if (ImGui::DragFloat3("Direction",&mEmitter.direction[0], 0.01f, -1.0f, 1.0f))
			{
				mEmitter.direction.Normalize();
			}
			if(ImGui::DragFloat("Spread", &mEmitter.spread, 0.01f, 0.0f, 1.0f))
			{
                mEmitter.spread = std::max(0.0f, mEmitter.spread);
			}

			if (ImGui::Button("Center"))
				mEmitter.translate = Vector3(0, 0, 0);
			ImGui::SameLine();
			if (ImGui::Button("Up"))
				mEmitter.translate.y += 1.0f;
			ImGui::SameLine();
			if (ImGui::Button("Down"))
				mEmitter.translate.y -= 1.0f;
		}

		// --- Appearance ---
		if (ImGui::CollapsingHeader("Appearance"))
		{
			const char* blendModeNames[] = { "None", "Normal", "AlphaAdd", "Screen" };
			int currentBlendMode = static_cast<int>(mBlendMode);
			if (ImGui::Combo("Blend Mode", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames)))
			{
				mBlendMode = static_cast<BlendMode>(currentBlendMode);
			}

			float startCol[4] = { mUpdate.colorStart.x, mUpdate.colorStart.y, mUpdate.colorStart.z, mUpdate.colorStart.w };
			float endCol[4]   = { mUpdate.colorEnd.x,   mUpdate.colorEnd.y,   mUpdate.colorEnd.z,   mUpdate.colorEnd.w };

			if (ImGui::ColorEdit4("Start Color", startCol))
			{
				mUpdate.colorStart = Vector4(startCol[0], startCol[1], startCol[2], startCol[3]);
			}
			if (ImGui::ColorEdit4("End Color", endCol))
			{
				mUpdate.colorEnd = Vector4(endCol[0], endCol[1], endCol[2], endCol[3]);
			}

			// Vertex quad scale (visual preview)
			static float quadScale = 1.0f;
			if (ImGui::SliderFloat("Quad Scale", &quadScale, 0.1f, 5.0f))
			{
				for (auto& v : mVerteices)
				{
					v.position.x *= quadScale;
					v.position.y *= quadScale;
				}
			}
		}

		if (ImGui::CollapsingHeader("Simulation"))
		{
			const char* modes[] = { "Default", "Gravity", "Wind", "Smoke", "Rain", "Snow", "Fire" };
			int mode = (int)mUpdate.updateMode;
			if (ImGui::Combo("Update Mode", &mode, modes, IM_ARRAYSIZE(modes)))
			{
				mUpdate.updateMode = (uint32_t)mode;
			}

			// Gravity Y
			float gravY = mUpdate.gravity.y;
			if (ImGui::SliderFloat("Gravity Y", &gravY, -50.0f, 50.0f))
			{
				mUpdate.gravity.y = gravY;
			}
			// Global drag
			if (ImGui::SliderFloat("Global Drag", &mUpdate.globalDrag, 0.0f, 5.0f))
			{
			}
			// Wind
			if (ImGui::DragFloat3("Wind", &mUpdate.wind.x, 0.1f))
			{
			}
			// Noise
			if (ImGui::SliderFloat("Noise Scale", &mUpdate.noiseScale, 0.0f, 10.0f))
			{
			}

			if (ImGui::Button("Reset Particles"))
			{
				Context.TransitionResource(mParticleBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				Context.TransitionResource(mParticleFreeList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				Context.TransitionResource(mParticleFreeListIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				Context.SetRootSignature(mParticleSig);
				Context.SetPipelineState(mParticleInitCS);
				Context.SetDynamicDescriptor(3, 0, mParticleBuffer.GetUAV());
				Context.SetDynamicDescriptor(3, 1, mParticleFreeList.GetUAV());
				Context.SetDynamicDescriptor(3, 2, mParticleFreeListIndex.GetUAV());
				Context.Dispatch((kMaxParticles + 255) / 256, 1, 1);
			}

		}

		if (ImGui::CollapsingHeader("Advanced"))
		{
			ImGui::Checkbox("Show Preset Textures", &openAdvanced);
			if (openAdvanced && !mPresets.empty())
			{
				for (size_t i = 0; i < mPresets.size(); ++i)
				{
					ImGui::Text("%u: %s -> %s", (uint32_t)i, mPresets[i].name.c_str(), mPresets[i].texturePath.c_str());
				}
			}
		}

		ImGui::End();

		// end ImGui

		Context.TransitionResource(mParticleBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(mParticleFreeList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(mParticleFreeListIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// emitter
		Context.SetRootSignature(mParticleSig);
		Context.SetPipelineState(mEmitterCS);
		Context.SetDynamicConstantBufferView(0, sizeof(mPrewView), &mPrewView);
		Context.SetDynamicConstantBufferView(1, sizeof(mEmitter), &mEmitter);
		Context.SetDynamicConstantBufferView(2, sizeof(mUpdate), &mUpdate);
		Context.SetDynamicDescriptor(3, 0, mParticleBuffer.GetUAV());
		Context.SetDynamicDescriptor(3, 1, mParticleFreeList.GetUAV());
		Context.SetDynamicDescriptor(3, 2, mParticleFreeListIndex.GetUAV());
		Context.Dispatch(1, 1, 1);

		// update
		Context.SetRootSignature(mParticleSig);
		Context.SetPipelineState(mParticleUpdateCS);
		Context.SetDynamicConstantBufferView(0, sizeof(mPrewView), &mPrewView);
		Context.SetDynamicConstantBufferView(1, sizeof(mEmitter), &mEmitter);
		Context.SetDynamicConstantBufferView(2, sizeof(mUpdate), &mUpdate);
		Context.SetDynamicDescriptor(3, 0, mParticleBuffer.GetUAV());
		Context.SetDynamicDescriptor(3, 1, mParticleFreeList.GetUAV());
		Context.SetDynamicDescriptor(3, 2, mParticleFreeListIndex.GetUAV());
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
		gfxContext.SetDynamicConstantBufferView(2, sizeof(mUpdate), &mUpdate);

		gfxContext.SetDynamicDescriptor(4, 0, mParticleBuffer.GetSRV());
		gfxContext.SetDynamicDescriptor(4, 1, tex.GetSRV());

		gfxContext.DrawInstanced((UINT)mVerteices.size(), mEmitter.count);
	}

	void ParticleSystem::Shutdown()
	{
		mVerteices.clear();
		mParticlePSOs.clear();
		mParticleBuffer.Destroy();
	}

}