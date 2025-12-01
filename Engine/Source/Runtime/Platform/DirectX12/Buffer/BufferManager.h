#pragma once
#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "ShadowBuffer.h"
#include "GpuBuffer.h"
#include "UploadBuffer.h"

namespace AtomEngine
{
    extern DepthBuffer gSceneDepthBuffer;  // D32_FLOAT_S8_UINT
    extern ColorBuffer gSceneColorBuffer;  // R11G11B10_FLOAT
    extern ColorBuffer gSceneNormalBuffer; // R16G16B16A16_FLOAT
    extern ColorBuffer gPostEffectsBuffer; // R32_UINT (to support Read-Modify-Write with a UAV)
    extern ColorBuffer gOverlayBuffer;
    
    extern ShadowBuffer gShadowBuffer;
    extern ShadowBuffer gRayTracedShadowBuffer;

    extern ColorBuffer gSSAOFullScreen;
    extern ColorBuffer gLinearDepth[2];

    extern ColorBuffer gLumaBuffer;
    extern ColorBuffer gBloomUAV1[2];	// 640x384 (1/3)
    extern ColorBuffer gBloomUAV2[2];	// 320x192 (1/6) 
    extern ColorBuffer gBloomUAV3[2];	// 160x96  (1/12)
    extern ColorBuffer gBloomUAV4[2];	// 80x48   (1/24)
    extern ColorBuffer gBloomUAV5[2];	// 40x24   (1/48)
    extern ColorBuffer gLumaLR;

    extern ByteAddressBuffer gHistogram;

    void InitializeBuffers(uint32_t bufferWidth, uint32_t bufferHeight);
    void DestroyBuffers();
    void OnResize(uint32_t NativeWidth, uint32_t NativeHeight);
}


