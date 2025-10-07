#pragma once
#include "RenderPass.h"

namespace AtomEngine
{
	class RenderPassManager
	{
	public:
		static RenderPassManager* GetInstance();

		void SetUpRenderPass();

	private:
		
		std::vector<std::unique_ptr<RenderPass>> mAllPasses;
		
		std::vector<RenderPass*> mCurrentPasses;

	private:
		RenderPassManager() = default;
        ~RenderPassManager() = default;
        RenderPassManager(const RenderPassManager&) = delete;
        RenderPassManager& operator=(const RenderPassManager&) = delete;
        RenderPassManager(RenderPassManager&&) = delete;
        RenderPassManager& operator=(RenderPassManager&&) = delete;

	};
}


