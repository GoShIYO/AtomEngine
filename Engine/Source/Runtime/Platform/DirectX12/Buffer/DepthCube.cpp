#include "DepthCube.h"
#include "../Core/DirectX12Core.h"

namespace AtomEngine
{
    using namespace DX12Core;

    void DepthCube::Create(const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, uint32_t arraySize)
    {
        D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, arraySize * 6, 1, Format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        D3D12_CLEAR_VALUE ClearValue = {};
        ClearValue.Format = Format;
        ClearValue.DepthStencil.Depth = mClearDepth;
        CreateTextureResource(gDevice.Get(), Name, ResourceDesc, ClearValue);
        CreateDerivedViews(gDevice.Get(), Format, arraySize);
    }

    void DepthCube::CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t arraySize)
    {
        ID3D12Resource* Resource = mResource.Get();

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = GetDSVFormat(Format);
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.MipSlice = 0;
        dsvDesc.Texture2DArray.ArraySize = 1;

        mHDSV.resize(arraySize * 6);
        for (uint32_t i = 0; i < arraySize * 6; ++i)
        {
            mHDSV[i] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            dsvDesc.Texture2DArray.FirstArraySlice = i;
            Device->CreateDepthStencilView(Resource, &dsvDesc, mHDSV[i]);
        }

        mHandleSRV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = GetDepthFormat(Format);
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.TextureCubeArray.NumCubes = arraySize;
        srvDesc.TextureCubeArray.MipLevels = 1;
        Device->CreateShaderResourceView(Resource, &srvDesc, mHandleSRV);
    }
}
