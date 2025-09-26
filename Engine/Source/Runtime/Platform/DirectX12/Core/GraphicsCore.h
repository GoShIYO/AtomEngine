#pragma once
#include "DescriptorAllocator.h"
#include "DescriptorHeap.h"
#include "../Pipline/RootSignature.h"

namespace AtomEngine
{
	class ContextManager;
	class CommandListManager;

	extern Microsoft::WRL::ComPtr<ID3D12Device> gDevice;
	extern CommandListManager gCommandManager;
	extern ContextManager gContextManager;

	extern bool gbTypedUAVLoadSupport_R11G11B10_FLOAT;
	extern bool gbTypedUAVLoadSupport_R16G16B16A16_FLOAT;

	void InitializeDx12(bool RequireDXRSupport = false);
	void ShutdownDx12();

	extern DescriptorAllocator gDescriptorAllocator[];
	inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1)
	{
		return gDescriptorAllocator[Type].Allocate(Count);
	}
}
