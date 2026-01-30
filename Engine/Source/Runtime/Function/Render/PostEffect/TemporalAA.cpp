#include "TemporalAA.h"
#include "Runtime/Platform/DirectX12/Context/ComputeContext.h"
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"
#include "Runtime/Platform/DirectX12/Pipeline/PipelineState.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Platform/DirectX12/Core/GraphicsCommon.h"
#include "Runtime/Platform/DirectX12/Shader/ShaderCompiler.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"

namespace AtomEngine
{
	bool EnableTAA = false;
	float Sharpness = 0.5f;
	float TemporalMaxLerp = 1.0f;
	float TemporalSpeedLimit = 64.0f;
	bool TriggerReset = false;

	ComputePSO sTemporalBlendCS(L"TAA: Temporal Blend CS");
	ComputePSO sBoundNeighborhoodCS(L"TAA: Bound Neighborhood CS");
	ComputePSO sSharpenTAACS(L"TAA: Sharpen TAA CS");
	ComputePSO sResolveTAACS(L"TAA: Resolve TAA CS");

	uint32_t sFrameIndex = 0;
	uint32_t sFrameIndexMod2 = 0;
	float sJitterX = 0.5f;
	float sJitterY = 0.5f;
	float sJitterDeltaX = 0.0f;
	float sJitterDeltaY = 0.0f;

	void TemporalAA::Initialize()
	{

		auto temporalBlendCS = ShaderCompiler::CompileBlob(L"PostEffects/TAA/TemporalBlend.hlsl", L"cs_6_6");
		auto boundNeighborhood = ShaderCompiler::CompileBlob(L"PostEffects/TAA/BoundNeighborhood.hlsl", L"cs_6_6");
		auto sharpenTAA = ShaderCompiler::CompileBlob(L"PostEffects/TAA/SharpenTAA.hlsl", L"cs_6_6");
		auto resolveTAA = ShaderCompiler::CompileBlob(L"PostEffects/TAA/ResolveTAA.hlsl", L"cs_6_6");

#define CreatePSO( ObjName, Shader ) \
		ObjName.SetRootSignature(gCommonRS); \
		ObjName.SetComputeShader(Shader.Get()); \
		ObjName.Finalize();

		CreatePSO(sTemporalBlendCS, temporalBlendCS);
		CreatePSO(sBoundNeighborhoodCS, boundNeighborhood);
		CreatePSO(sSharpenTAACS, sharpenTAA);
		CreatePSO(sResolveTAACS, resolveTAA);
	}
	void TemporalAA::Shutdown()
	{
	}
	void TemporalAA::Update(uint64_t frameIndex)
	{
		sFrameIndex = static_cast<uint32_t>(frameIndex);
		sFrameIndexMod2 = sFrameIndex % 2;
		if (EnableTAA)// && !DepthOfField::Enable)
		{
			static const float Halton23[8][2] =
			{
				{ 0.0f / 8.0f, 0.0f / 9.0f }, { 4.0f / 8.0f, 3.0f / 9.0f },
				{ 2.0f / 8.0f, 6.0f / 9.0f }, { 6.0f / 8.0f, 1.0f / 9.0f },
				{ 1.0f / 8.0f, 4.0f / 9.0f }, { 5.0f / 8.0f, 7.0f / 9.0f },
				{ 3.0f / 8.0f, 2.0f / 9.0f }, { 7.0f / 8.0f, 5.0f / 9.0f }
			};

			const float* Offset = nullptr;

			// CBRでは、奇数個のジッター位置を持つことが効果的です。なぜなら、
			// 奇数フレームと偶数フレームの両方ですべてのサンプル位置を探索できるからです。
			//（また、最も役に立たないサンプルは、4ピクセルのちょうど中央に位置する最初のサンプルです。）
				Offset = Halton23[sFrameIndex % 8];

			sJitterDeltaX = sJitterX - Offset[0];
			sJitterDeltaY = sJitterY - Offset[1];
			sJitterX = Offset[0];
			sJitterY = Offset[1];
		}
		else
		{
			sJitterDeltaX = sJitterX - 0.5f;
			sJitterDeltaY = sJitterY - 0.5f;
			sJitterX = 0.5f;
			sJitterY = 0.5f;
		}

	}
	void TemporalAA::GetJitterOffset(float& JitterX, float& JitterY)
	{
		JitterX = sJitterX;
		JitterY = sJitterY;
	}
	void TemporalAA::ClearHistory(CommandContext& Context)
	{
		GraphicsContext& gfxContext = Context.GetGraphicsContext();

		if (EnableTAA)
		{
			gfxContext.TransitionResource(gTemporalColor[0], D3D12_RESOURCE_STATE_RENDER_TARGET);
			gfxContext.TransitionResource(gTemporalColor[1], D3D12_RESOURCE_STATE_RENDER_TARGET, true);
			gfxContext.ClearColor(gTemporalColor[0]);
			gfxContext.ClearColor(gTemporalColor[1]);
		}
	}
	void TemporalAA::ResolveImage(CommandContext& BaseContext)
	{
		ComputeContext& Context = BaseContext.GetComputeContext();

		static bool s_EnableTAA = false;

		if (EnableTAA != s_EnableTAA && TriggerReset)
		{
			ClearHistory(Context);
			s_EnableTAA = EnableTAA;
			TriggerReset = false;
		}

		uint32_t Src = sFrameIndexMod2;
		uint32_t Dst = Src ^ 1;

		{
			ApplyTemporalAA(Context);
			SharpenImage(Context, gTemporalColor[Dst]);
		}
	}
	void TemporalAA::ApplyTemporalAA(ComputeContext& Context)
	{
		uint32_t Src = sFrameIndexMod2;
		uint32_t Dst = Src ^ 1;

		Context.SetRootSignature(gCommonRS);
		Context.SetPipelineState(sTemporalBlendCS);

		__declspec(align(16)) struct ConstantBuffer
		{
			float RcpBufferDim[2];
			float TemporalBlendFactor;
			float RcpSeedLimiter;
			float CombinedJitter[2];
		};
		ConstantBuffer cbv = {
			1.0f / gSceneColorBuffer.GetWidth(), 1.0f / gSceneColorBuffer.GetHeight(),
			(float)TemporalMaxLerp, 1.0f / TemporalSpeedLimit,
			sJitterDeltaX, sJitterDeltaY
		};

		Context.SetDynamicConstantBufferView(3, sizeof(cbv), &cbv);

		Context.TransitionResource(gVelocityBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.TransitionResource(gTemporalColor[Src], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.TransitionResource(gTemporalColor[Dst], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(gLinearDepth[Src], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.TransitionResource(gLinearDepth[Dst], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.SetDynamicDescriptor(1, 0, gVelocityBuffer.GetSRV());
		Context.SetDynamicDescriptor(1, 1, gSceneColorBuffer.GetSRV());
		Context.SetDynamicDescriptor(1, 2, gTemporalColor[Src].GetSRV());
		Context.SetDynamicDescriptor(1, 3, gLinearDepth[Src].GetSRV());
		Context.SetDynamicDescriptor(1, 4, gLinearDepth[Dst].GetSRV());
		Context.SetDynamicDescriptor(2, 0, gTemporalColor[Dst].GetUAV());

		Context.Dispatch2D(gSceneColorBuffer.GetWidth(), gSceneColorBuffer.GetHeight(), 16, 8);
	}
	void TemporalAA::SharpenImage(ComputeContext& Context, ColorBuffer& TemporalColor)
	{
		Context.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(TemporalColor, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		Context.SetPipelineState(Sharpness >= 0.001f ? sSharpenTAACS : sResolveTAACS);
		Context.SetConstants(0, 1.0f + Sharpness, 0.25f * Sharpness);
		Context.SetDynamicDescriptor(1, 0, TemporalColor.GetSRV());
		Context.SetDynamicDescriptor(2, 0, gSceneColorBuffer.GetUAV());
		Context.Dispatch2D(gSceneColorBuffer.GetWidth(), gSceneColorBuffer.GetHeight());
	}
}