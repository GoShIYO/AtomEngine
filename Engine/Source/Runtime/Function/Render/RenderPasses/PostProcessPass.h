#pragma once
#include "RenderPass.h"
#include "Runtime/Function/Render/Particle/ParticleSystem.h"
#include "Runtime/Platform/DirectX12/Pipline/RootSignature.h"
#include "../GridRenderer.h"

namespace AtomEngine
{
	class PostProcessPass : public RenderPass
	{
	public:
		PostProcessPass();
		~PostProcessPass();
		void Initialize() override;
		void Update(GraphicsContext& Context, float deltaTime) override;
		void Render(GraphicsContext& gfxContext) override;

	private:
		//システム
		std::unique_ptr<GridRenderer> mGridRenderer;

	private:
		//PSO
		RootSignature mPostEffectsRS;
		ComputePSO mToneMapCS;
		ComputePSO mApplyBloomCS;
		ComputePSO mBloomCS;
		ComputePSO mBlurCS;
		ComputePSO mAdaptExposureCS;
		ComputePSO mDownsampleBloomCS;
		ComputePSO mUpsampleAndBlurCS;
		ComputePSO mBloomExtractAndDownsampleCS;
		ComputePSO mGenerateHistogramCS;
		ComputePSO mExtractLumaCS;

		StructuredBuffer mExposureBuffer;

	private:
		//パラメータ
		bool mEnableAdaptation = true;
		float mMinExposure = 1 / 64.0f;
		float mMaxExposure = 8.0f;
		float mTargetLuminance = 0.08f;
		float mAdaptationRate = 0.05f;
		float mExposure = 2.0f;

		bool mEnableBloom = true;
		float mBloomThreshold = 4.0f;
		float mBloomStrength = 0.1f;
		float mBloomUpsampleFactor = 0.65f;

	private:

		void GenerateBloom(ComputeContext& Context);
		void ExtractLuma(ComputeContext& Context);
		void UpdateExposure(ComputeContext& Context);
		void BlurBuffer(ComputeContext& Context, ColorBuffer buffer[2], const ColorBuffer& lowerResBuf, float upsampleBlendFactor);
		void ProcessEffects(ComputeContext& Context);
	};
}


