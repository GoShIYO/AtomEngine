#include "PostProcessPass.h"

namespace AtomEngine
{
	PostProcessPass::PostProcessPass()
	{
		mParticleSystem.reset(new ParticleSystem());
		mParticleSystem->Initialize();
	}

	void PostProcessPass::Update(GraphicsContext& Context, float deltaTime)
	{
		mParticleSystem->Update(Context.GetComputeContext(), deltaTime);
	}

	void PostProcessPass::Render(GraphicsContext& gfxContext)
	{
		mParticleSystem->Render(gfxContext, *mCamera);
	}
}

