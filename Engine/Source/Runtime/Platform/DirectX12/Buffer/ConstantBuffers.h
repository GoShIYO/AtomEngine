#pragma once
#include "Runtime/Core/Math/Matrix4x4.h"

namespace AtomEngine
{
    __declspec(align(256)) struct MeshConstants
    {
        Matrix4x4 worldMatrix;              // World matrix
        Matrix4x4 worldInverseTranspose;    // World matrix inverse transpose(法線用)
    };

    __declspec(align(256)) struct MaterialConstants
    {
        float baseColorFactor[4] = { 1,1,1,1 };    // default=[1,1,1,1]
        float emissiveFactor[3] = { 0,0,0 };       // default=[0,0,0]
        float normalTextureScale = 1.0f;           // default=1
        float metallicFactor = 0.0f;               // default=0
        float roughnessFactor = 0.5f;              // default=0.5
        union
        {
            uint32_t flags;
            struct
            {
                //各テクスチャのUV
                uint32_t baseColorUV : 1;
                uint32_t metallicRoughnessUV : 1;
                uint32_t occlusionUV : 1;
                uint32_t emissiveUV : 1;
                uint32_t normalUV : 1;

                //3つの特別モード
                uint32_t twoSided : 1;
                uint32_t alphaTest : 1;
                uint32_t alphaBlend : 1;

                uint32_t pad0 : 8;

                uint32_t alphaRef : 16;// 半精度浮動小数
            };
        };
    };

    __declspec(align(256)) struct GlobalConstants
    {
        Matrix4x4 ViewProjMatrix;   // ビュー・プロジェクション行列
        Matrix4x4 SunShadowMatrix;  // light space行列
        Vector3 CameraPos;          // カメラ位置
        Vector3 SunDirection;       // メイン平行光源の方向
        Vector3 SunIntensity;       // メイン平行光源の強度
        float IBLRange;             // IBLの範囲
        float IBLBias;              // IBLのバイアス
    };
}


