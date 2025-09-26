#pragma once
#include "../D3dUtility/d3dInclude.h"

namespace AtomEngine
{
    class RootSignature;

    class IndirectParameter
    {
        friend class CommandSignature;
    public:

        IndirectParameter()
        {
            mIndirectParam.Type = (D3D12_INDIRECT_ARGUMENT_TYPE)0xFFFFFFFF;
        }

        void Draw(void)
        {
            mIndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
        }

        void DrawIndexed(void)
        {
            mIndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
        }

        void Dispatch(void)
        {
            mIndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
        }

        void VertexBufferView(UINT Slot)
        {
            mIndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
            mIndirectParam.VertexBuffer.Slot = Slot;
        }

        void IndexBufferView(void)
        {
            mIndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
        }

        void Constant(UINT RootParameterIndex, UINT DestOffsetIn32BitValues, UINT Num32BitValuesToSet)
        {
            mIndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
            mIndirectParam.Constant.RootParameterIndex = RootParameterIndex;
            mIndirectParam.Constant.DestOffsetIn32BitValues = DestOffsetIn32BitValues;
            mIndirectParam.Constant.Num32BitValuesToSet = Num32BitValuesToSet;
        }

        void ConstantBufferView(UINT RootParameterIndex)
        {
            mIndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
            mIndirectParam.ConstantBufferView.RootParameterIndex = RootParameterIndex;
        }

        void ShaderResourceView(UINT RootParameterIndex)
        {
            mIndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
            mIndirectParam.ShaderResourceView.RootParameterIndex = RootParameterIndex;
        }

        void UnorderedAccessView(UINT RootParameterIndex)
        {
            mIndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
            mIndirectParam.UnorderedAccessView.RootParameterIndex = RootParameterIndex;
        }

        const D3D12_INDIRECT_ARGUMENT_DESC& GetDesc(void) const { return mIndirectParam; }

    protected:

        D3D12_INDIRECT_ARGUMENT_DESC mIndirectParam;
    };

    class CommandSignature
    {
    public:

        CommandSignature(UINT NumParams = 0) : mFinalized(FALSE), kNumParameters(NumParams)
        {
            Reset(NumParams);
        }

        void Destroy(void)
        {
            mSignature = nullptr;
            mParamArray = nullptr;
        }

        void Reset(UINT NumParams)
        {
            if (NumParams > 0)
                mParamArray.reset(new IndirectParameter[NumParams]);
            else
                mParamArray = nullptr;

            kNumParameters = NumParams;
        }

        IndirectParameter& operator[] (size_t EntryIndex)
        {
            ASSERT(EntryIndex < kNumParameters);
            return mParamArray.get()[EntryIndex];
        }

        const IndirectParameter& operator[] (size_t EntryIndex) const
        {
            ASSERT(EntryIndex < kNumParameters);
            return mParamArray.get()[EntryIndex];
        }

        void Finalize(const RootSignature* RootSignature = nullptr);

        ID3D12CommandSignature* GetSignature() const { return mSignature.Get(); }

    protected:

        BOOL mFinalized;
        UINT kNumParameters;
        std::unique_ptr<IndirectParameter[]> mParamArray;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> mSignature;
    };
}
