#pragma once
#include "PixelBuffer.h"

namespace AtomEngine
{
    class DepthBuffer : public PixelBuffer
    {
    public:
        DepthBuffer(float ClearDepth = 0.0f, uint8_t ClearStencil = 0)
            : mClearDepth(ClearDepth), mClearStencil(ClearStencil)
        {
            mHandleDSV[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
            mHandleDSV[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
            mHandleDSV[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
            mHandleDSV[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
            mHandleDepthSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
            mHandleStencilSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        }

        // 深度バッファを作成します。アドレスが指定された場合、メモリは割り当てられません。
        void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format,
            D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

        void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format,
            D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

        // 事前に作成されたCPUハンドルを取得する
        const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV() const { return mHandleDSV[0]; }
        const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_DepthReadOnly() const { return mHandleDSV[1]; }
        const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_StencilReadOnly() const { return mHandleDSV[2]; }
        const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_ReadOnly() const { return mHandleDSV[3]; }
        const D3D12_CPU_DESCRIPTOR_HANDLE& GetDepthSRV() const { return mHandleDepthSRV; }
        const D3D12_CPU_DESCRIPTOR_HANDLE& GetStencilSRV() const { return mHandleStencilSRV; }

        float GetClearDepth() const { return mClearDepth; }
        uint8_t GetClearStencil() const { return mClearStencil; }

    protected:

        void CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format);

        float mClearDepth;
        uint8_t mClearStencil;
        D3D12_CPU_DESCRIPTOR_HANDLE mHandleDSV[4];
        D3D12_CPU_DESCRIPTOR_HANDLE mHandleDepthSRV;
        D3D12_CPU_DESCRIPTOR_HANDLE mHandleStencilSRV;
    };
}


