#include "PostProcessPass.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Function/Render/Particle/ParticleSystem.h"
#include "imgui.h"

namespace AtomEngine
{
	PostProcessPass::PostProcessPass()
	{

	}

	void PostProcessPass::Initialize()
	{
		ParticleSystem::Initialize();

		mPostEffect = std::make_unique<PostEffect>();
		mPostEffect->Initialize();

		mGridRenderer = std::make_unique<GridRenderer>();
		mGridRenderer->Initialize();
	}

	void PostProcessPass::Update(GraphicsContext& Context, float deltaTime)
	{
		ParticleSystem::Update(Context.GetComputeContext(), deltaTime);
	}

	void PostProcessPass::Render(GraphicsContext& gfxContext)
	{
		ParticleSystem::Render(gfxContext, *mCamera, *mRTV, *mDSV);

		mPostEffect->Render(gfxContext.GetComputeContext());
#ifdef _DEBUG
		//mGridRenderer->Render(gfxContext, mCamera, *mRTV, *mDSV, mViewport, mScissor);
#endif
	}
}

