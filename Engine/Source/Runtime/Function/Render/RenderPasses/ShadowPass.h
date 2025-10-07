#pragma once
#include "RenderPass.h"

namespace AtomEngine
{
	class ShadowPass :
		public RenderPass
	{
	public:
		~ShadowPass() override = default;

		void Setup(GraphicsContext& context) override{}
		void Execute(GraphicsContext& context, RenderQueue& queue, GlobalConstants& globals) override;
		void Cleanup(GraphicsContext& context) override {}

	private:

	};
}


