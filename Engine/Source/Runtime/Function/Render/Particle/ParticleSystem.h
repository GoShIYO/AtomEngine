#pragma once
#include "Particle.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "Runtime/Platform/DirectX12/Context/ComputeContext.h"
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"
#include <map>
namespace AtomEngine
{
	class ParticleSystem
	{
		friend class Particle;
	public:

		static void Initialize();

		static void Update(ComputeContext& Context, float deltaTime);

		static void Render(GraphicsContext& gfxContext, const Camera& camera, ColorBuffer& ColorTarget, DepthBuffer& DepthTarget);

		static void Shutdown();

		static uint32_t CreateParticle(ParticleProperty& props);
		static uint32_t CreateParticlePrefab(const ParticleProperty& props);
		static uint32_t EmitPrefab(uint32_t prefabId, const Vector3& emitPosW);
		static void ResetParticle(uint32_t particleId);
		static float GetCurrentLife(uint32_t particleId);
		static void SetBlendMode(BlendMode mode) { sBlendMode = mode; }
		static EmitterProperty& GetEmitterProperty(uint32_t particleId) { return sParticlePool[particleId]->GetProperty().EmitProperties; }
		static void EmitParticle(uint32_t particleId, const Vector3& emitPosW);
	private:
		static RootSignature mParticleSig;
		static std::vector<GraphicsPSO> mParticlePSOs;
		static StructuredBuffer sParticleBuffer;
		static DescriptorHandle sParticleGpuHandle;

		static ComputePSO sParticleEmitCS;
		static ComputePSO sParticleUpdateCS;
		static ComputePSO sParticleDispatchIndirectArgsCS;
		static ComputePSO sParticleFinalDispatchIndirectArgsCS;

		static IndirectArgsBuffer sDrawIndirectArgs;
		static IndirectArgsBuffer sDispatchIndirectArgs;
		static IndirectArgsBuffer sFinalDispatchIndirectArgs;

		static std::vector<std::unique_ptr<Particle>> sParticlePool;
		static std::vector<Particle*> sParticlesActive;
		static BlendMode sBlendMode;
		static PrewView sPrewView;

		static DescriptorHeap sTextureArrayHeap;
		static std::vector < std::pair<DescriptorHandle, TextureRef>> sTextures;
		static std::map<std::wstring, uint32_t> sTextureArrayLookup;
		static bool sInitComplete;
		static std::vector<ParticleProperty> sParticlePrefabs;
	private:
		static void SetFinalBuffers(ComputeContext& CompContext);
		static uint32_t GetTextureIndex(const std::wstring& name);
	public:
		static std::vector<Particle*>& GetParticles(void) { return sParticlesActive; }
		static uint32_t CreateParticleFromFile(const std::string& utf8Path);
		static bool ParseParticlePropertyFromJson(const std::string& text, ParticleProperty& out);
	};
}

