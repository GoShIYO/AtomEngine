#pragma once
#include "GpuResource.h"

namespace AtomEngine
{
    class PixelBuffer : public GpuResource
    {
    public:
        PixelBuffer() : mWidth(0), mHeight(0), mArraySize(0), mFormat(DXGI_FORMAT_UNKNOWN){}

        uint32_t GetWidth(void) const { return mWidth; }
        uint32_t GetHeight(void) const { return mHeight; }
        uint32_t GetDepth(void) const { return mArraySize; }
        const DXGI_FORMAT& GetFormat(void) const { return mFormat; }

        // 生のピクセルバッファの内容をファイルに書き込みます
        // データの前に16バイトのヘッダーが付くことに注意: { DXGI_FORMAT, ピッチ（ピクセル単位）, 幅（ピクセル単位）, 高さ }
        void ExportToFile(const std::wstring& FilePath);

    protected:

        D3D12_RESOURCE_DESC DescribeTex2D(uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize, uint32_t NumMips, DXGI_FORMAT Format, UINT Flags);
        //リソースとの関連付け
        void AssociateWithResource(const std::wstring& Name, ID3D12Resource* Resource, D3D12_RESOURCE_STATES CurrentState);
        //テクスチャリソースを作成
        void CreateTextureResource(ID3D12Device* Device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc,
            D3D12_CLEAR_VALUE ClearValue, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

        static DXGI_FORMAT GetBaseFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetUAVFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetDSVFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetDepthFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetStencilFormat(DXGI_FORMAT Format);
        static size_t BytesPerPixel(DXGI_FORMAT Format);

        uint32_t mWidth;
        uint32_t mHeight;
        uint32_t mArraySize;
        DXGI_FORMAT mFormat;
    };
}
