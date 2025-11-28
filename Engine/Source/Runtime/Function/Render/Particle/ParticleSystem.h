#pragma once
#include "Particle.h"
#include "Runtime/Resource/TextureRef.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "Runtime/Platform/DirectX12/Context/ComputeContext.h"
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"

namespace AtomEngine
{
	class ParticleSystem
	{
	public:

		void Initialize();

		void Update(ComputeContext& Context,float deltaTime);

		void Render(GraphicsContext& gfxContext,const Camera& camera);

		void Shutdown();
	private:
		RootSignature mParticleSig;

		ComputePSO mParticleInitCS;
		ComputePSO mParticleUpdateCS;
		ComputePSO mEmitterCS;
		ComputePSO mParticleFreeListCS;
		ComputePSO mParticleFreeListIndexCS;

		std::vector<GraphicsPSO> mParticlePSOs;
		
		StructuredBuffer mParticleBuffer;
		StructuredBuffer mParticleFreeList;
		StructuredBuffer mParticleFreeListIndex;

		IndirectArgsBuffer mDrawIndirectArgs;

		std::vector<ParticleVertex>mVerteices;
		
		std::vector<std::unique_ptr<Particle>> mParticlePool;
		std::vector<Particle*> mParticlesActive;

		TextureRef tex;

		BlendMode mBlendMode = BlendMode::kBlendAlphaAdd;

		float mEmitTimer = 0.0f;
		EmitterConstants mEmitter;
		PrewView mPrewView;
	};
}


