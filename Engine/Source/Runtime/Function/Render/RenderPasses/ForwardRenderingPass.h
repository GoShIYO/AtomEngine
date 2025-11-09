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
		Vector3 mSunDirection = { 0,-1,0 };
		float mSunOrientation = -0.5f;
		float mSunInclination = 0.75f;
		Vector3 mShadowCenter = { 0.0f,5,0.0f };
		Vector3 mShadowBounds = { 15, 20, 20 };
		float mIBLBias = 2.0f;
	private:
		void RenderObjects(RenderQueue& queue);
	};
}


