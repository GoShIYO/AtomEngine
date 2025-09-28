#include "DescriptorHeap.h"
#include "DirectX12Core.h"

namespace AtomEngine
{
    using namespace DX12Core;

    void DescriptorHeap::Create(const std::wstring& Name, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxCount)
    {
        mHeapDesc.Type = Type;
        mHeapDesc.NumDescriptors = MaxCount;
        mHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        mHeapDesc.NodeMask = 1;

        ThrowIfFailed(gDevice->CreateDescriptorHeap(&mHeapDesc, IID_PPV_ARGS(mHeap.ReleaseAndGetAddressOf())));

#ifndef _DEBUG
        (void)Name;
#else
        mHeap->SetName(Name.c_str());
#endif

        mDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(mHeapDesc.Type);
        mNumFreeDescriptors = mHeapDesc.NumDescriptors;
        mFirstHandle = DescriptorHandle(
            mHeap->GetCPUDescriptorHandleForHeapStart(),
            mHeap->GetGPUDescriptorHandleForHeapStart());
        mNextFreeHandle = mFirstHandle;
    }

    DescriptorHandle DescriptorHeap::Alloc(uint32_t Count)
    {
        ASSERT(HasAvailableSpace(Count), "Descriptor Heap out of space.  Increase heap size.");
        DescriptorHandle ret = mNextFreeHandle;
        mNextFreeHandle += Count * mDescriptorSize;
        mNumFreeDescriptors -= Count;
        return ret;
    }

    bool DescriptorHeap::ValidateHandle(const DescriptorHandle& DHandle) const
    {
        if (DHandle.GetCpuPtr() < mFirstHandle.GetCpuPtr() ||
            DHandle.GetCpuPtr() >= mFirstHandle.GetCpuPtr() + mHeapDesc.NumDescriptors * mDescriptorSize)
            return false;

        if (DHandle.GetGpuPtr() - mFirstHandle.GetGpuPtr() !=
            DHandle.GetCpuPtr() - mFirstHandle.GetCpuPtr())
            return false;

        return true;
    }
}