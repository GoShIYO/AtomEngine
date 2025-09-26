#pragma once
#include "../D3dUtility/d3dInclude.h"
#include <mutex>

namespace AtomEngine
{
    class DescriptorAllocator
    {
    public:
        DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE Type) :
            mType(Type), mCurrentHeap(nullptr), mDescriptorSize(0)
        {
            mCurrentHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32_t Count);

        static void DestroyAll(void);

    protected:

        static const uint32_t kNumDescriptorsPerHeap = 256;
        static std::mutex mAllocationMutex;
        static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> mDescriptorHeapPool;
        static ID3D12DescriptorHeap* RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type);

        D3D12_DESCRIPTOR_HEAP_TYPE mType;
        ID3D12DescriptorHeap* mCurrentHeap;
        D3D12_CPU_DESCRIPTOR_HANDLE mCurrentHandle;
        uint32_t mDescriptorSize;
        uint32_t mRemainingFreeHandles = 0;
    };
}   


