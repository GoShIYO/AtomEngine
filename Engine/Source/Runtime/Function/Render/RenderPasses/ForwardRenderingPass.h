#pragma once
#include "RenderPass.h"
#include "../SkyboxRenderer.h"
#include "../GridRenderer.h"
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
		GridRenderer mGrid;

		ShadowCamera mShadowCamera;

		float mSunLightIntensity = 1.0f;
		Vector3 mSunDirection = { -0.505f,-0.707f,-0.505f };
		float mSunOrientation = -0.5f;
		float mSunInclination = 0.75f;
		Vector3 mShadowCenter = { 0.0f,60.0f,0.0f };
		Vector3 mShadowBounds = { 240, 120, 120 };
		float mIBLBias = 2.0f;
		float mIBLFactor = 0.5f;
		bool mEnableIBL = false;

	private:
		void RenderObjects(RenderQueue& queue);
	};
}


