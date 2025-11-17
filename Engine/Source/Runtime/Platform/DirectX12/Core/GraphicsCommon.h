#pragma once
#include "../Core/SamplerManager.h"

namespace AtomEngine
{
    class SamplerDesc;
    class CommandSignature;
    class RootSignature;
    class ComputePSO;
    class GraphicsPSO;

    void InitializeCommonState(void);
    void DestroyCommonState(void);

    extern SamplerDesc SamplerLinearWrapDesc;
    extern SamplerDesc SamplerAnisoWrapDesc;
    extern SamplerDesc SamplerShadowDesc;
    extern SamplerDesc SamplerLinearClampDesc;
    extern SamplerDesc SamplerVolumeWrapDesc;
    extern SamplerDesc SamplerPointClampDesc;
    extern SamplerDesc SamplerPointBorderDesc;
    extern SamplerDesc SamplerLinearBorderDesc;

    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearWrap;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerAnisoWrap;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerShadow;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearClamp;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerVolumeWrap;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointClamp;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointBorder;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearBorder;

    extern D3D12_RASTERIZER_DESC RasterizerDefault;
    extern D3D12_RASTERIZER_DESC RasterizerDefaultMsaa;
    extern D3D12_RASTERIZER_DESC RasterizerDefaultCw;
    extern D3D12_RASTERIZER_DESC RasterizerDefaultCwMsaa;
    extern D3D12_RASTERIZER_DESC RasterizerTwoSided;
    extern D3D12_RASTERIZER_DESC RasterizerTwoSidedMsaa;
    extern D3D12_RASTERIZER_DESC RasterizerShadow;
    extern D3D12_RASTERIZER_DESC RasterizerShadowCW;
    extern D3D12_RASTERIZER_DESC RasterizerShadowTwoSided;

    extern D3D12_BLEND_DESC BlendNoColorWrite;		    // XXX
    extern D3D12_BLEND_DESC BlendDisable;			    // 1, 0
    extern D3D12_BLEND_DESC BlendPreMultiplied;		    // 1, 1-SrcA
    extern D3D12_BLEND_DESC BlendTraditional;		    // SrcA, 1-SrcA
    extern D3D12_BLEND_DESC BlendAdditive;			    // 1, 1
    extern D3D12_BLEND_DESC BlendTraditionalAdditive;   // SrcA, 1

    extern D3D12_DEPTH_STENCIL_DESC DepthStateDisabled;
    extern D3D12_DEPTH_STENCIL_DESC DepthStateReadWrite;
    extern D3D12_DEPTH_STENCIL_DESC DepthStateReadOnly;
    extern D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyReversed;
    extern D3D12_DEPTH_STENCIL_DESC DepthStateTestEqual;

    extern CommandSignature DispatchIndirectCommandSignature;
    extern CommandSignature DrawIndirectCommandSignature;

    enum eDefaultTexture
    {
        kMagenta2D = 0,         // 欠けているテクスチャを示す
        kBlackOpaque2D,         // 黒いテクスチャ
        kBlackTransparent2D,    // 透過する黒いテクスチャ
        kWhiteOpaque2D,         // 白いテクスチャ
        kWhiteTransparent2D,    // 透過する白いテクスチャ
        kDefaultNormalMap,      // デフォルトの法線マップ
        kBlackCubeMap,          // 黒いキューブマップ
        kDefaultBRDFLUT,        // デフォルトのBRDF LUT

        kNumDefaultTextures     // デフォルトテクスチャの数
    };
    D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultTexture(eDefaultTexture texID);

    enum BlendMode
    {
        kBlendNone,
        kBlendNormal,
        kBlendAlphaAdd,
        kBlendScreen,

        kNumBlendModes,
    };
    extern std::unordered_map<BlendMode, D3D12_BLEND_DESC> gBlendModeTable;

    extern RootSignature gCommonRS;

}

