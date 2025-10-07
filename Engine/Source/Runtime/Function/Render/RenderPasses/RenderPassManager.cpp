#include "RenderPassManager.h"
#include "RenderPassManager.h"

namespace AtomEngine
{
	RenderPassManager* RenderPassManager::GetInstance()
	{
		static RenderPassManager instance;
		return &instance;
	}

	void RenderPassManager::SetUpRenderPass()
	{

	}
}

