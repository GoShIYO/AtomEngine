#pragma once
#include "PixelBuffer.h"

namespace AtomEngine
{
    class DepthCube : public PixelBuffer
    {
    public:
        DepthCube(float ClearDepth = 0.0f)
            : mClearDepth(ClearDepth)
        {
            mHDSV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
            mHandleSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        }

        void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, uint32_t arraySize);

        const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV(int arrayIndex, int faceIndex) const { return mHDSV[arrayIndex * 6 + faceIndex]; }
        const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return mHandleSRV; }

        float GetClearDepth() const { return mClearDepth; }

    protected:

        void CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t arraySize);

        float mClearDepth;
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> mHDSV;
        D3D12_CPU_DESCRIPTOR_HANDLE mHandleSRV;
    };
}
