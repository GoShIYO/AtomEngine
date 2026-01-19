#pragma once
#include "CameraBase.h"

namespace AtomEngine
{
    class ShadowCamera : public CameraBase
    {
    public:
        ShadowCamera() = default;

        void UpdateMatrix(
            Vector3 lightDir,        // 光に平行な方向、進行方向
            Vector3 shadowCenter,    // 影領域の遠方境界面上の中心位置
            Vector3 shadowBounds,    // シャドウバッファによって表されるワールド空間における幅、高さ、深度
            uint32_t BufferWidth,           // シャドウバッファの幅
            uint32_t BufferHeight           // シャドウバッファの高さ（通常は幅と同じ）
        );
        const Matrix4x4& GetShadowMatrix() const
        {
            return mShadowMatrix;
        }
    private:
        Matrix4x4 mShadowMatrix;
    };
}


