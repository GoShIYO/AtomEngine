#include "PiplineState.h"
#include "Runtime/Core/Utility/Hash.h"
#include "../Core/DirectX12Core.h"
#include <map>
#include <thread>
#include <mutex>

using Microsoft::WRL::ComPtr;

namespace AtomEngine
{
	using namespace DX12Core;

	static std::map< size_t, ComPtr<ID3D12PipelineState> > sGraphicsPSOHashMap;
	static std::map< size_t, ComPtr<ID3D12PipelineState> > sComputePSOHashMap;

	void PSO::DestroyAll(void)
	{
		sGraphicsPSOHashMap.clear();
		sComputePSOHashMap.clear();
	}


	GraphicsPSO::GraphicsPSO(const wchar_t* Name)
		: PSO(Name)
	{
		ZeroMemory(&mPSODesc, sizeof(mPSODesc));
		mPSODesc.NodeMask = 1;
		mPSODesc.SampleMask = 0xFFFFFFFFu;
		mPSODesc.SampleDesc.Count = 1;
		mPSODesc.InputLayout.NumElements = 0;
	}

	void GraphicsPSO::SetBlendState(const D3D12_BLEND_DESC& BlendDesc)
	{
		mPSODesc.BlendState = BlendDesc;
	}

	void GraphicsPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc)
	{
		mPSODesc.RasterizerState = RasterizerDesc;
	}

	void GraphicsPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc)
	{
		mPSODesc.DepthStencilState = DepthStencilDesc;
	}

	void GraphicsPSO::SetSampleMask(UINT SampleMask)
	{
		mPSODesc.SampleMask = SampleMask;
	}

	void GraphicsPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
	{
		ASSERT(TopologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, "Can't draw with undefined topology");
		mPSODesc.PrimitiveTopologyType = TopologyType;
	}

	void GraphicsPSO::SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps)
	{
		mPSODesc.IBStripCutValue = IBProps;
	}

	void GraphicsPSO::SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
	{
		SetRenderTargetFormats(0, nullptr, DSVFormat, MsaaCount, MsaaQuality);
	}

	void GraphicsPSO::SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
	{
		SetRenderTargetFormats(1, &RTVFormat, DSVFormat, MsaaCount, MsaaQuality);
	}

	void GraphicsPSO::SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
	{
		ASSERT(NumRTVs == 0 || RTVFormats != nullptr, "Null format array conflicts with non-zero length");
		for (UINT i = 0; i < NumRTVs; ++i)
		{
			ASSERT(RTVFormats[i] != DXGI_FORMAT_UNKNOWN);
			mPSODesc.RTVFormats[i] = RTVFormats[i];
		}
		for (UINT i = NumRTVs; i < mPSODesc.NumRenderTargets; ++i)
			mPSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
		mPSODesc.NumRenderTargets = NumRTVs;
		mPSODesc.DSVFormat = DSVFormat;
		mPSODesc.SampleDesc.Count = MsaaCount;
		mPSODesc.SampleDesc.Quality = MsaaQuality;
	}

	void GraphicsPSO::SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs)
	{
		mPSODesc.InputLayout.NumElements = NumElements;

		if (NumElements > 0)
		{
			D3D12_INPUT_ELEMENT_DESC* NewElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * NumElements);
			memcpy(NewElements, pInputElementDescs, NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
			mInputLayouts.reset((const D3D12_INPUT_ELEMENT_DESC*)NewElements);
		}
		else
			mInputLayouts = nullptr;
	}

	void GraphicsPSO::Finalize()
	{
		// ルートシグネチャーが確定していることを確認
		mPSODesc.pRootSignature = mRootSignature->GetSignature();
		ASSERT(mPSODesc.pRootSignature != nullptr);

		mPSODesc.InputLayout.pInputElementDescs = nullptr;

		size_t HashCode = HashState(&mPSODesc);
		HashCode = HashState(mInputLayouts.get(), mPSODesc.InputLayout.NumElements, HashCode);
		mPSODesc.InputLayout.pInputElementDescs = mInputLayouts.get();

		ID3D12PipelineState** PSORef = nullptr;
		bool firstCompile = false;
		{
			static std::mutex s_HashMapMutex;
			std::lock_guard<std::mutex> CS(s_HashMapMutex);
			auto iter = sGraphicsPSOHashMap.find(HashCode);

			// 次回の問い合わせで誰かが先にここに到着したことがわかるように、スペースを予約しておいく。
			if (iter == sGraphicsPSOHashMap.end())
			{
				firstCompile = true;
				PSORef = sGraphicsPSOHashMap[HashCode].GetAddressOf();
			}
			else
				PSORef = iter->second.GetAddressOf();
		}

		if (firstCompile)
		{
			ASSERT(mPSODesc.DepthStencilState.DepthEnable != (mPSODesc.DSVFormat == DXGI_FORMAT_UNKNOWN));
			ThrowIfFailed(gDevice->CreateGraphicsPipelineState(&mPSODesc, IID_PPV_ARGS(&mPSO)));
			sGraphicsPSOHashMap[HashCode].Attach(mPSO);
			mPSO->SetName(mName);
		}
		else
		{
			while (*PSORef == nullptr)
				std::this_thread::yield();
			mPSO = *PSORef;
		}
	}

	void ComputePSO::Finalize()
	{
		// ルートシグネチャーが確定していることを確認
		mPSODesc.pRootSignature = mRootSignature->GetSignature();
		ASSERT(mPSODesc.pRootSignature != nullptr);

		size_t HashCode = HashState(&mPSODesc);

		ID3D12PipelineState** PSORef = nullptr;
		bool firstCompile = false;
		{
			static std::mutex s_HashMapMutex;
			std::lock_guard<std::mutex> CS(s_HashMapMutex);
			auto iter = sComputePSOHashMap.find(HashCode);

			// 次回の問い合わせで誰かが先にここに到着したことがわかるように、スペースを予約しておいく。
			if (iter == sComputePSOHashMap.end())
			{
				firstCompile = true;
				PSORef = sComputePSOHashMap[HashCode].GetAddressOf();
			}
			else
				PSORef = iter->second.GetAddressOf();
		}

		if (firstCompile)
		{
			ThrowIfFailed(gDevice->CreateComputePipelineState(&mPSODesc, IID_PPV_ARGS(&mPSO)));
			sComputePSOHashMap[HashCode].Attach(mPSO);
			mPSO->SetName(mName);
		}
		else
		{
			while (*PSORef == nullptr)
				std::this_thread::yield();
			mPSO = *PSORef;
		}
	}

	ComputePSO::ComputePSO(const wchar_t* Name)
		: PSO(Name)
	{
		ZeroMemory(&mPSODesc, sizeof(mPSODesc));
		mPSODesc.NodeMask = 1;
	}
}