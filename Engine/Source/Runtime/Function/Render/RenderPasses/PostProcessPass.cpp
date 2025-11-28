#include "PostProcessPass.h"

namespace AtomEngine
{
	PostProcessPass::PostProcessPass()
	{
		mParticleSystem.reset(new ParticleSystem());
		mParticleSystem->Initialize();

		mGridRenderer.reset(new GridRenderer());
		mGridRenderer->Initialize();
	}

	void PostProcessPass::Update(GraphicsContext& Context, float deltaTime)
	{
		mParticleSystem->Update(Context.GetComputeContext(), deltaTime);
	}

	void PostProcessPass::Render(GraphicsContext& gfxContext)
	{
		mParticleSystem->Render(gfxContext, *mCamera);
		mGridRenderer->Render(gfxContext, mCamera, *mRTV, *mDSV, mViewport, mScissor);
	}
}

