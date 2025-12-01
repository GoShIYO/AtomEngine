#include "PostProcessPass.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Platform/DirectX12/Shader/ShaderCompiler.h"
#include "imgui.h"

namespace AtomEngine
{
	PostProcessPass::PostProcessPass()
	{

	}

	PostProcessPass::~PostProcessPass()
	{
		mExposureBuffer.Destroy();


		ParticleSystem::Shutdown();
	}

	constexpr float kInitialMinLog = -12.0f;
	constexpr float kInitialMaxLog = 4.0f;

	void PostProcessPass::Initialize()
	{
		mPostEffectsRS.Reset(4, 2);
		mPostEffectsRS.InitStaticSampler(0, SamplerLinearClampDesc);
		mPostEffectsRS.InitStaticSampler(1, SamplerLinearBorderDesc);
		mPostEffectsRS[0].InitAsConstants(0, 5);
		mPostEffectsRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 4);
		mPostEffectsRS[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4);
		mPostEffectsRS[3].InitAsConstantBuffer(1);
		mPostEffectsRS.Finalize(L"Post Effects");

		auto toneMapCS = ShaderCompiler::CompileBlob(L"PostEffects/ToneMap.hlsl", L"cs_6_2");
		auto applyBloomCS = ShaderCompiler::CompileBlob(L"PostEffects/ApplyBloom.hlsl", L"cs_6_2");
		auto generateHistogramCS = ShaderCompiler::CompileBlob(L"PostEffects/GenerateHistogram.hlsl", L"cs_6_2");
		auto adaptExposureCS = ShaderCompiler::CompileBlob(L"PostEffects/AdaptExposure.hlsl", L"cs_6_2");
		auto downsampleBloomCS = ShaderCompiler::CompileBlob(L"PostEffects/DownsampleBloom.hlsl", L"cs_6_2");
		auto upsampleAndBlurCS = ShaderCompiler::CompileBlob(L"PostEffects/UpsampleAndBlur.hlsl", L"cs_6_2");
		auto blurCS = ShaderCompiler::CompileBlob(L"PostEffects/Blur.hlsl", L"cs_6_2");
		auto bloomExtractAndDownsampleCS = ShaderCompiler::CompileBlob(L"PostEffects/BloomExtractAndDownsample.hlsl", L"cs_6_2");
		auto extractLumaCS = ShaderCompiler::CompileBlob(L"PostEffects/ExtractLuma.hlsl", L"cs_6_2");

#define CreatePSO( ObjName, ShaderByteCode ) \
		ObjName.SetRootSignature(mPostEffectsRS); \
		ObjName.SetComputeShader(ShaderByteCode.Get()); \
		ObjName.Finalize();

		CreatePSO(mToneMapCS, toneMapCS);
		CreatePSO(mApplyBloomCS, applyBloomCS);
		CreatePSO(mGenerateHistogramCS, generateHistogramCS);
		CreatePSO(mAdaptExposureCS, adaptExposureCS);
		CreatePSO(mDownsampleBloomCS, downsampleBloomCS);
		CreatePSO(mUpsampleAndBlurCS, upsampleAndBlurCS);
		CreatePSO(mBlurCS, blurCS);
		CreatePSO(mBloomExtractAndDownsampleCS, bloomExtractAndDownsampleCS);
		CreatePSO(mExtractLumaCS, extractLumaCS);

#undef CreatePSO

		__declspec(align(16)) float initExposure[] =
		{
			mExposure, 1.0f / mExposure, mExposure, 0.0f,
			kInitialMinLog, kInitialMaxLog, kInitialMaxLog - kInitialMinLog, 1.0f / (kInitialMaxLog - kInitialMinLog)
		};

		mExposureBuffer.Create(L"Exposure", 8, 4, initExposure);

		mMinExposure = 0.0f;
		mMaxExposure = 2.0f;
		mAdaptationRate = 0.05f;
		mExposure = 2.0f;
		mBloomThreshold = 0.3f;
		mBloomStrength = 1.0f;
		mBloomUpsampleFactor = 0.65f;

		ParticleSystem::Initialize();
		mGridRenderer.reset(new GridRenderer());
		mGridRenderer->Initialize();
	}

	void PostProcessPass::Update(GraphicsContext& Context, float deltaTime)
	{
		ParticleSystem::Update(Context.GetComputeContext(), deltaTime);

		ImGui::Begin("Post Process");
		ImGui::Checkbox("EnableAdaptation", &mEnableAdaptation);
		ImGui::DragFloat("MinExposure", &mMinExposure, 0.01f, -8.0f, 0.0f);
		ImGui::DragFloat("MaxExposure", &mMaxExposure, 0.01f, 0.0f, 8.0f);
		ImGui::DragFloat("TargetLuminance", &mTargetLuminance, 0.01f, 0.01f, 0.99f);
		ImGui::DragFloat("AdatationRate", &mAdaptationRate, 0.01f, 0.01f, 1.0f);
		ImGui::DragFloat("Exposure", &mExposure, 0.01f, -8.0f, 8.0f);

		ImGui::Separator();

		ImGui::Checkbox("EnableBloom", &mEnableBloom);
		ImGui::DragFloat("BloomThreshold", &mBloomThreshold, 0.01f, 0.0f, 8.0f);
		ImGui::DragFloat("BloomStrength", &mBloomStrength, 0.01f, 0.0f, 2.0f);
		ImGui::DragFloat("BloomUpsampleFactor", &mBloomUpsampleFactor, 0.01f, 0.0f, 1.0f);
		ImGui::End();
	}

	void PostProcessPass::Render(GraphicsContext& gfxContext)
	{
		ParticleSystem::Render(gfxContext, *mCamera, *mRTV, *mDSV);

		ComputeContext& Context = gfxContext.GetComputeContext();
		Context.SetRootSignature(mPostEffectsRS);
		Context.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		ProcessEffects(Context);

		mGridRenderer->Render(gfxContext, mCamera, *mRTV, *mDSV, mViewport, mScissor);
	}

	void PostProcessPass::GenerateBloom(ComputeContext& Context)
	{
		uint32_t kBloomWidth = gLumaLR.GetWidth();
		uint32_t kBloomHeight = gLumaLR.GetHeight();

		// これらのブルームバッファのサイズは、128で割り切れるという優れた数値と、およそ16:9であることから選択されました。
		// ぼかしアルゴリズムは9ピクセル×9ピクセルなので、各ピクセルのアスペクト比が正方形でない場合、ぼかしは
		// 円形ではなく楕円形になります。偶然にも、これらは720pバッファの1/2、1080pバッファの1/3にほぼ相当します。
		// これは、コンソール上のブルームバッファの一般的なサイズです。
		ASSERT(kBloomWidth % 16 == 0 && kBloomHeight % 16 == 0, "Bloom buffer dimensions must be multiples of 16");

		Context.SetConstants(0, 1.0f / kBloomWidth, 1.0f / kBloomHeight, mBloomThreshold);
		Context.TransitionResource(gBloomUAV1[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(gLumaLR, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.TransitionResource(mExposureBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		{
			Context.SetDynamicDescriptor(1, 0, gBloomUAV1[0].GetUAV());
			Context.SetDynamicDescriptor(1, 1, gLumaLR.GetUAV());
			Context.SetDynamicDescriptor(2, 0, gSceneColorBuffer.GetSRV());
			Context.SetDynamicDescriptor(2, 1, mExposureBuffer.GetSRV());

			Context.SetPipelineState(mBloomExtractAndDownsampleCS);
			Context.Dispatch2D(kBloomWidth, kBloomHeight);
		}

		Context.TransitionResource(gBloomUAV1[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.SetDynamicDescriptor(2, 0, gBloomUAV1[0].GetSRV());

		Context.TransitionResource(gBloomUAV2[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(gBloomUAV3[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(gBloomUAV4[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(gBloomUAV5[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		D3D12_CPU_DESCRIPTOR_HANDLE UAVs[4] = {
			gBloomUAV2[0].GetUAV(), gBloomUAV3[0].GetUAV(), gBloomUAV4[0].GetUAV(), gBloomUAV5[0].GetUAV() };
		Context.SetDynamicDescriptors(1, 0, 4, UAVs);

		//各ディスパッチ グループは 8x8 スレッドですが、各スレッドは 2x2 ソーステクセルを読み取ります。
		Context.SetPipelineState(mDownsampleBloomCS);
		Context.Dispatch2D(kBloomWidth / 2, kBloomHeight / 2);

		Context.TransitionResource(gBloomUAV2[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.TransitionResource(gBloomUAV3[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.TransitionResource(gBloomUAV4[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.TransitionResource(gBloomUAV5[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		//ぼかし、アップサンプリング、そして4回ぼかしをかける
		BlurBuffer(Context, gBloomUAV5, gBloomUAV5[0], 1.0f);
		BlurBuffer(Context, gBloomUAV4, gBloomUAV5[1], mBloomUpsampleFactor);
		BlurBuffer(Context, gBloomUAV3, gBloomUAV4[1], mBloomUpsampleFactor);
		BlurBuffer(Context, gBloomUAV2, gBloomUAV3[1], mBloomUpsampleFactor);
		BlurBuffer(Context, gBloomUAV1, gBloomUAV2[1], mBloomUpsampleFactor);
	}

	void PostProcessPass::ExtractLuma(ComputeContext& Context)
	{
		Context.TransitionResource(gLumaLR, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(mExposureBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.SetConstants(0, 1.0f / gLumaLR.GetWidth(), 1.0f / gLumaLR.GetHeight());
		Context.SetDynamicDescriptor(1, 0, gLumaLR.GetUAV());
		Context.SetDynamicDescriptor(2, 0, gSceneColorBuffer.GetSRV());
		Context.SetDynamicDescriptor(2, 1, mExposureBuffer.GetSRV());
		Context.SetPipelineState(mExtractLumaCS);
		Context.Dispatch2D(gLumaLR.GetWidth(), gLumaLR.GetHeight());
	}
	void PostProcessPass::UpdateExposure(ComputeContext& Context)
	{
		if (!mEnableAdaptation)
		{
			__declspec(align(16)) float initExposure[] =
			{
				mExposure, 1.0f / mExposure, mExposure, 0.0f,
				kInitialMinLog, kInitialMaxLog, kInitialMaxLog - kInitialMinLog, 1.0f / (kInitialMaxLog - kInitialMinLog)
			};
			Context.WriteBuffer(mExposureBuffer, 0, initExposure, sizeof(initExposure));
			Context.TransitionResource(mExposureBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			return;
		}

		// HDRヒストグラムを作成
		Context.TransitionResource(gHistogram, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
		Context.ClearUAV(gHistogram);
		Context.TransitionResource(gLumaLR, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.SetDynamicDescriptor(1, 0, gHistogram.GetUAV());
		Context.SetDynamicDescriptor(2, 0, gLumaLR.GetSRV());
		Context.SetPipelineState(mGenerateHistogramCS);
		Context.SetConstants(0, gLumaLR.GetHeight());
		Context.Dispatch2D(gLumaLR.GetWidth(), gLumaLR.GetHeight(), 16, gLumaLR.GetHeight());

		__declspec(align(16)) struct
		{
			float TargetLuminance;
			float AdaptationRate;
			float MinExposure;
			float MaxExposure;
			uint32_t PixelCount;
		} constants =
		{
			mTargetLuminance, mAdaptationRate, mMinExposure, mMaxExposure,
			gLumaLR.GetWidth() * gLumaLR.GetHeight()
		};
		Context.TransitionResource(gHistogram, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.TransitionResource(mExposureBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.SetDynamicDescriptor(1, 0, mExposureBuffer.GetUAV());
		Context.SetDynamicDescriptor(2, 0, gHistogram.GetSRV());
		Context.SetDynamicConstantBufferView(3, sizeof(constants), &constants);
		Context.SetPipelineState(mAdaptExposureCS);
		Context.Dispatch();
		Context.TransitionResource(mExposureBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}
	void PostProcessPass::BlurBuffer(ComputeContext& Context, ColorBuffer buffer[2], const ColorBuffer& lowerResBuf, float upsampleBlendFactor)
	{
		uint32_t bufferWidth = buffer[0].GetWidth();
		uint32_t bufferHeight = buffer[0].GetHeight();
		Context.SetConstants(0, 1.0f / bufferWidth, 1.0f / bufferHeight, upsampleBlendFactor);

		Context.TransitionResource(buffer[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.SetDynamicDescriptor(1, 0, buffer[1].GetUAV());
		D3D12_CPU_DESCRIPTOR_HANDLE SRVs[2] = { buffer[0].GetSRV(), lowerResBuf.GetSRV() };
		Context.SetDynamicDescriptors(2, 0, 2, SRVs);

		Context.SetPipelineState(&buffer[0] == &lowerResBuf ? mBlurCS : mUpsampleAndBlurCS);

		Context.Dispatch2D(bufferWidth, bufferHeight);

		Context.TransitionResource(buffer[1], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}
	void PostProcessPass::ProcessEffects(ComputeContext& Context)
	{
		if (mEnableBloom)
		{
			GenerateBloom(Context);
			Context.TransitionResource(gBloomUAV1[1], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
		else if (mEnableAdaptation)
			ExtractLuma(Context);

		Context.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		Context.TransitionResource(gLumaBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(mExposureBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		Context.SetPipelineState(mToneMapCS);

		//セットコンスタンス
		Context.SetConstants(0, 1.0f / gSceneColorBuffer.GetWidth(), 1.0f / gSceneColorBuffer.GetHeight(), mBloomStrength);

		Context.SetDynamicDescriptor(1, 0, gSceneColorBuffer.GetUAV());

		Context.SetDynamicDescriptor(1, 1, gLumaBuffer.GetUAV());

		//HDR値とぼかしブルームバッファを読み込む
		Context.SetDynamicDescriptor(2, 0, mExposureBuffer.GetSRV());
		Context.SetDynamicDescriptor(2, 1, mEnableBloom ? gBloomUAV1[1].GetSRV() : GetDefaultTexture(kBlackOpaque2D));

		Context.Dispatch2D(gSceneColorBuffer.GetWidth(), gSceneColorBuffer.GetHeight());

		//これを最後に行うと、明るいパスはトーンマッピングと同じエクスポージャー出力
		UpdateExposure(Context);
	}
}

