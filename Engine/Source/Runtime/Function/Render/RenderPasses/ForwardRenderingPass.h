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

		Color mSunColor = Color::White;
		float mSunLightIntensity = 1.0f;
		Vector3 mSunDirection = { -0.003f,-0.988f,0.156f };
		Vector3 mShadowCenter = { 0.0f,0,0.0f };
		Vector3 mShadowDim = { 160, 160, 160 };
		float mIBLBias = 2.0f;
		float mIBLFactor = 1.0f;
		float mShadowBias = 0.001f;
		bool mEnableIBL = true;
		DescriptorHandle mShadowSrvGpuHandle;
			
	private:
		void RenderObjects(RenderQueue& queue);
	};
}


