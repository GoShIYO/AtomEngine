#include "PostProcessPass.h"

namespace AtomEngine
{
	PostProcessPass::PostProcessPass()
	{
		ParticleSystem::Initialize();
		mGridRenderer.reset(new GridRenderer());
		mGridRenderer->Initialize();
	}

	PostProcessPass::~PostProcessPass()
	{
        ParticleSystem::Shutdown();
	}

	void PostProcessPass::Update(GraphicsContext& Context, float deltaTime)
	{
		ParticleSystem::Update(Context.GetComputeContext(), deltaTime);
	}

	void PostProcessPass::Render(GraphicsContext& gfxContext)
	{
		ParticleSystem::Render(gfxContext, *mCamera,*mRTV,*mDSV);
		mGridRenderer->Render(gfxContext, mCamera, *mRTV, *mDSV, mViewport, mScissor);
	}
}

