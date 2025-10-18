#pragma once
#include "CommandContext.h"

namespace AtomEngine
{
    class ComputeContext : public CommandContext
    {
    public:

        static ComputeContext& Begin(const std::wstring& ID = L"", bool Async = false);

        void ClearUAV(GpuBuffer& Target);
        void ClearUAV(ColorBuffer& Target);

        void SetRootSignature(const RootSignature& RootSig);

        void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants);
        void SetConstant(UINT RootIndex, UINT Offset, DWParam Val);
        void SetConstants(UINT RootIndex, DWParam X);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);
        void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
        void SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData);
        void SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData);
        void SetBufferSRV(UINT RootIndex, const GpuBuffer& SRV, UINT64 Offset = 0);
        void SetBufferUAV(UINT RootIndex, const GpuBuffer& UAV, UINT64 Offset = 0);
        void SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);

        void SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
        void SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);

        void Dispatch(size_t GroupCountX = 1, size_t GroupCountY = 1, size_t GroupCountZ = 1);
        void Dispatch1D(size_t ThreadCountX, size_t GroupSizeX = 64);
        void Dispatch2D(size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX = 8, size_t GroupSizeY = 8);
        void Dispatch3D(size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX, size_t GroupSizeY, size_t GroupSizeZ);
        void DispatchIndirect(GpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset = 0);
        void ExecuteIndirect(CommandSignature& CommandSig, GpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0,
            uint32_t MaxCommands = 1, GpuBuffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0);
    };

    inline void ComputeContext::SetRootSignature(const RootSignature& RootSig)
    {
        if (RootSig.GetSignature() == mCurComputeRootSignature)
            return;

        mCommandList->SetComputeRootSignature(mCurComputeRootSignature = RootSig.GetSignature());

        mDynamicViewDescriptorHeap.ParseComputeRootSignature(RootSig);
        mDynamicSamplerDescriptorHeap.ParseComputeRootSignature(RootSig);
    }

    inline void ComputeContext::SetConstantArray(UINT RootEntry, UINT NumConstants, const void* pConstants)
    {
        mCommandList->SetComputeRoot32BitConstants(RootEntry, NumConstants, pConstants, 0);
    }

    inline void ComputeContext::SetConstant(UINT RootEntry, UINT Offset, DWParam Val)
    {
        mCommandList->SetComputeRoot32BitConstant(RootEntry, Val.Uint, Offset);
    }

    inline void ComputeContext::SetConstants(UINT RootEntry, DWParam X)
    {
        mCommandList->SetComputeRoot32BitConstant(RootEntry, X.Uint, 0);
    }

    inline void ComputeContext::SetConstants(UINT RootEntry, DWParam X, DWParam Y)
    {
        mCommandList->SetComputeRoot32BitConstant(RootEntry, X.Uint, 0);
        mCommandList->SetComputeRoot32BitConstant(RootEntry, Y.Uint, 1);
    }

    inline void ComputeContext::SetConstants(UINT RootEntry, DWParam X, DWParam Y, DWParam Z)
    {
        mCommandList->SetComputeRoot32BitConstant(RootEntry, X.Uint, 0);
        mCommandList->SetComputeRoot32BitConstant(RootEntry, Y.Uint, 1);
        mCommandList->SetComputeRoot32BitConstant(RootEntry, Z.Uint, 2);
    }

    inline void ComputeContext::SetConstants(UINT RootEntry, DWParam X, DWParam Y, DWParam Z, DWParam W)
    {
        mCommandList->SetComputeRoot32BitConstant(RootEntry, X.Uint, 0);
        mCommandList->SetComputeRoot32BitConstant(RootEntry, Y.Uint, 1);
        mCommandList->SetComputeRoot32BitConstant(RootEntry, Z.Uint, 2);
        mCommandList->SetComputeRoot32BitConstant(RootEntry, W.Uint, 3);
    }
    inline void ComputeContext::SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
    {
        mCommandList->SetComputeRootConstantBufferView(RootIndex, CBV);
    }

    inline void ComputeContext::SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && IsAligned(BufferData, 16));
        DynAlloc cb = mCpuLinearAllocator.Allocate(BufferSize);
        //SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
        memcpy(cb.DataPtr, BufferData, BufferSize);
        mCommandList->SetComputeRootConstantBufferView(RootIndex, cb.GpuAddress);
    }

    inline void ComputeContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && IsAligned(BufferData, 16));
        DynAlloc cb = mCpuLinearAllocator.Allocate(BufferSize);
        SIMDMemCopy(cb.DataPtr, BufferData, AlignUp(BufferSize, 16) >> 4);
        mCommandList->SetComputeRootShaderResourceView(RootIndex, cb.GpuAddress);
    }

    inline void ComputeContext::SetBufferSRV(UINT RootIndex, const GpuBuffer& SRV, UINT64 Offset)
    {
        ASSERT((SRV.mUsageState & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0);
        mCommandList->SetComputeRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
    }

    inline void ComputeContext::SetBufferUAV(UINT RootIndex, const GpuBuffer& UAV, UINT64 Offset)
    {
        ASSERT((UAV.mUsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
        mCommandList->SetComputeRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
    }

    inline void ComputeContext::Dispatch(size_t GroupCountX, size_t GroupCountY, size_t GroupCountZ)
    {
        FlushResourceBarriers();
        mDynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(mCommandList);
        mDynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(mCommandList);
        mCommandList->Dispatch((UINT)GroupCountX, (UINT)GroupCountY, (UINT)GroupCountZ);
    }

    inline void ComputeContext::Dispatch1D(size_t ThreadCountX, size_t GroupSizeX)
    {
        Dispatch(DivideByMultiple(ThreadCountX, GroupSizeX), 1, 1);
    }

    inline void ComputeContext::Dispatch2D(size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX, size_t GroupSizeY)
    {
        Dispatch(
            DivideByMultiple(ThreadCountX, GroupSizeX),
            DivideByMultiple(ThreadCountY, GroupSizeY), 1);
    }

    inline void ComputeContext::Dispatch3D(size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX, size_t GroupSizeY, size_t GroupSizeZ)
    {
        Dispatch(
            DivideByMultiple(ThreadCountX, GroupSizeX),
            DivideByMultiple(ThreadCountY, GroupSizeY),
            DivideByMultiple(ThreadCountZ, GroupSizeZ));
    }


    inline void ComputeContext::SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
    {
        SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
    }

    inline void ComputeContext::SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
    {
        mDynamicViewDescriptorHeap.SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
    }

    inline void ComputeContext::SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
    {
        SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
    }

    inline void ComputeContext::SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
    {
        mDynamicSamplerDescriptorHeap.SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
    }


    inline void ComputeContext::SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
    {
        mCommandList->SetComputeRootDescriptorTable(RootIndex, FirstHandle);
    }

    inline void ComputeContext::ExecuteIndirect(CommandSignature& CommandSig,
        GpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset,
        uint32_t MaxCommands, GpuBuffer* CommandCounterBuffer, uint64_t CounterOffset)
    {
        FlushResourceBarriers();
        mDynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(mCommandList);
        mDynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(mCommandList);
        mCommandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands,
            ArgumentBuffer.GetResource(), ArgumentStartOffset,
            CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(), CounterOffset);
    }

    inline void ComputeContext::DispatchIndirect(GpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset)
    {
        ExecuteIndirect(DispatchIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
    }
}
