#pragma once
#include "RenderPass.h"
#include "Runtime/Function/Camera/ShadowCamera.h"

#include "../SkyboxRenderer.h"

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
		Vector3 mSunDirection = { -0.14f,-0.446f,0.884f };
		float mSunOrientation = -0.5f;
		float mSunInclination = 0.75f;
		Vector3 mShadowCenter = { 0.0f,0,0.0f };
		Vector3 mShadowDim = { 60, 60, 60 };
		float mIBLBias = 2.0f;
		float mIBLFactor = 1.0f;
		bool mEnableIBL = true;
		DescriptorHandle mGpuHandle;

	private:
		void RenderObjects(RenderQueue& queue);
	};
}


