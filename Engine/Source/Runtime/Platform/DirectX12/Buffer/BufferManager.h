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

    extern ColorBuffer gSSAOFullScreen;
    extern ColorBuffer gLinearDepth[2];

    void InitializeBuffers(uint32_t bufferWidth, uint32_t bufferHeight);
    void DestroyBuffers();
    void OnResize(uint32_t NativeWidth, uint32_t NativeHeight);
}


