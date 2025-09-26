#include "GraphicsCore.h"
#include "CommandListManager.h"
#include "SwapChain.h"
#include "Runtime/Core/Utility/Utility.h"
#include "../Context/CommandContext.h"
#include "../Buffer/BufferManager.h"

#pragma comment(lib, "d3d12.lib") 
#pragma comment(lib, "dxgi.lib") 
#pragma comment(lib, "dxguid.lib")

using namespace Microsoft::WRL;
namespace AtomEngine
{
	ComPtr<ID3D12Device> gDevice = nullptr;
	CommandListManager gCommandManager;
	ContextManager gContextManager;

	DescriptorAllocator gDescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	};

	bool gbTypedUAVLoadSupport_R11G11B10_FLOAT = false;
	bool gbTypedUAVLoadSupport_R16G16B16A16_FLOAT = false;

	void InitializeDx12(bool RequireDXRSupport)
	{
		uint32_t useDebugLayers = 0;

#if _DEBUG
		// デバッグビルドの場合はデフォルトでtrue
		useDebugLayers = 1;
#endif

		DWORD dxgiFactoryFlags = 0;

		if (useDebugLayers)
		{
			ComPtr<ID3D12Debug1> debugController = nullptr;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
				debugController->SetEnableGPUBasedValidation(TRUE);
			}
			else
			{
				Log(L"[WARNING]:  Unable to enable D3D12 debug validation layer\n");
			}

#if _DEBUG
			ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
			{
				dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

				DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
				{
					80 /* 
					   IDXGISwapChain::GetContainingOutput: 
					   スワップチェーンのアダプターは、スワップチェーンのウィンドウが存在する出力を制御しません 
					   */,
				};
				DXGI_INFO_QUEUE_FILTER filter = {};
				filter.DenyList.NumIDs = _countof(hide);
				filter.DenyList.pIDList = hide;
				dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
			}
#endif
		}


		Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;

		ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory)));

		// 使用するアダプタ用の変数。最初にnullptrを入れておく
		ComPtr <IDXGIAdapter4> useAdapter = nullptr;
		// 良い順でアダプタをを頼む
		for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
			DXGI_ERROR_NOT_FOUND; i++)
		{
			// アダプタの情報を取得する
			DXGI_ADAPTER_DESC3 adapterDesc{};
			ThrowIfFailed(useAdapter->GetDesc3(&adapterDesc));
			if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
			{
				// ソフトウェアアダプタでなければ採用
				Log(L"Use Adapter:%s\n", adapterDesc.Description);
				break;
			}
			useAdapter = nullptr;
		}
		ASSERT(useAdapter != nullptr);

		// デバイスの生成
		D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_12_2,
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0
		};
		const char* featureLevelStrings[] = {
			"12.2",
			"12.1",
			"12.0"
		};
		// 高い順で生成できるか試す
		for (size_t i = 0; i < _countof(featureLevels); i++)
		{
			// 採用したアダプターでデバイスを生成
			HRESULT hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&gDevice));
			if (SUCCEEDED(hr))
			{
				Log("feature levels is:%s", featureLevelStrings[i]);
				if (RequireDXRSupport)
				{
					D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
					HRESULT hrDXR = gDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
					if (FAILED(hrDXR) || options5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
					{
						Log("[ERROR]: DXR is required but not supported on this device.\n");
						continue;
					}
					else
					{
						Log("DXR supported: Tier %d\n", options5.RaytracingTier);
					}
				}

				break;
			}
		}

		ASSERT(gDevice != nullptr);
#ifdef _DEBUG
		ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
		if (SUCCEEDED(gDevice->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
			// ヤバイエラー時に止まる
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			// エラー時に止まる
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			//警告時に止まる
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

			//抑制するメッセージのID
			D3D12_MESSAGE_ID denyIds[] = {
				//Windows11のDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
			};
			//抑制するレベル
			D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
			D3D12_INFO_QUEUE_FILTER filter{};
			filter.DenyList.NumIDs = _countof(denyIds);
			filter.DenyList.pIDList = denyIds;
			filter.DenyList.NumSeverities = _countof(severities);
			filter.DenyList.pSeverityList = severities;
			//指定したメッセージの表示を抑制する
			infoQueue->PushStorageFilter(&filter);

		}
#endif
		D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData = {};
		if (SUCCEEDED(gDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(FeatureData))))
		{
			if (FeatureData.TypedUAVLoadAdditionalFormats)
			{
				D3D12_FEATURE_DATA_FORMAT_SUPPORT Support =
				{
					DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE
				};

				if (SUCCEEDED(gDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
					(Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
				{
					gbTypedUAVLoadSupport_R11G11B10_FLOAT = true;
				}

				Support.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

				if (SUCCEEDED(gDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
					(Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
				{
					gbTypedUAVLoadSupport_R16G16B16A16_FLOAT = true;
				}
			}
		}

		gCommandManager.Create(gDevice.Get());
		InitializeCommonState();
		SwapChain::Initialize();

	}

	void ShutdownDx12()
	{
		gCommandManager.IdleGPU();
		CommandContext::DestroyAllContexts();
		gCommandManager.Shutdown();

		PSO::DestroyAll();
		RootSignature::DestroyAll();
		DescriptorAllocator::DestroyAll();
		DestroyCommonState();
		DestroyBuffers();
		SwapChain::Finalize();

		gDevice.Reset();

		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		}
	}
}