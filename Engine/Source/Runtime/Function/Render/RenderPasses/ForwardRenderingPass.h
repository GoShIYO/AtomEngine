#pragma once
#include "RenderPass.h"
#include "../SkyboxRenderer.h"
#include "Runtime/Function/Camera/ShadowCamera.h"

namespace AtomEngine
{
	class ForwardRenderingPass : public RenderPass
	{
	public:
		ForwardRenderingPass();
		~ForwardRenderingPass();

        void Render(GraphicsContext& gfxContext) override;

	private:
		SkyboxRenderer mSkybox;
		ShadowCamera mShadowCamera;

		float mSunLightIntensity = 4.0f;
		float mSunOrientation = -0.5f;
		float mSunInclination = 0.75f;
		float mIBLBias = 2.0f;
	};
}


