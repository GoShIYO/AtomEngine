#pragma once
#include "Particle.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "Runtime/Platform/DirectX12/Context/ComputeContext.h"
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"

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
		static void ResetParticle(uint32_t particleId);
		static float GetCurrentLife(uint32_t particleId);
		static void RegisterTexture(uint32_t particleId, const TextureRef& texture);
		static void SetBlendMode(BlendMode mode) { sBlendMode = mode; }
	private:
		static RootSignature mParticleSig;
		static std::vector<GraphicsPSO> mParticlePSOs;
		static StructuredBuffer sParticleBuffer;

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

		static bool sInitComplete;

	private:
		static void SetFinalBuffers(ComputeContext& CompContext);
	public:
		static std::vector<Particle*>& GetParticles(void) {return sParticlesActive;}
	};
}

