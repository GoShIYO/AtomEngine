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

		float mSunLightIntensity = 1.0f;
		Vector3 mSunDirection = { 0,-1,0 };
		float mSunOrientation = -0.5f;
		float mSunInclination = 0.75f;
		Vector3 mShadowCenter = { 0.0f,10.0f,0.0f };
		Vector3 mShadowBounds = { 25, 20, 20 };
		float mIBLBias = 2.0f;
		float mIBLFactor = 0.5f;
	private:
		void RenderObjects(RenderQueue& queue);
	};
}


