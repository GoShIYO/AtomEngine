#include "ShadowPass.h"

namespace AtomEngine
{

	void ShadowPass::Execute(GraphicsContext& context, RenderQueue& queue, GlobalConstants& globals)
	{
		//RenderQueue shadowQueue(RenderQueue::kShadows);
		//shadowQueue.SetCamera(*mCamera);
		//shadowQueue.SetDepthStencilTarget(*mDSV);

		////TODO: モデルインスタンスを描画する
		//

		//shadowQueue.Sort();
		//shadowQueue.RenderMeshes(RenderQueue::kZPass, context, globals);
	}

}