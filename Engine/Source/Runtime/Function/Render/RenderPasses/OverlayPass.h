#pragma once
#include "RenderPass.h"

namespace AtomEngine
{
	class OverlayPass : public RenderPass
	{
	public:

		void Render(GraphicsContext& gfxContext) override;
	};
}


