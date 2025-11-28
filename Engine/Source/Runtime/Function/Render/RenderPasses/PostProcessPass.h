#pragma once
#include "RenderPass.h"
#include "Runtime/Function/Render/Particle/ParticleSystem.h"

namespace AtomEngine
{
	class PostProcessPass : public RenderPass
	{
	public:
		PostProcessPass();

		void Update(GraphicsContext& Context, float deltaTime) override;
		void Render(GraphicsContext& gfxContext) override;

	private:
		std::unique_ptr<ParticleSystem> mParticleSystem;

	};
}


