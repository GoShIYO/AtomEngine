#pragma once

#ifdef __cplusplus
#include "Runtime/Core/Math/MathInclude.h"
using namespace AtomEngine;
using float4 = Vector4;
using float3 = Vector3;
using float2 = Vector2;
using float4x4 = Matrix4x4;
using float3x3 = Matrix3x3;
using uint = uint32_t;
#endif

struct alignas(16) MeshConstants
{
    Matrix4x4 worldMatrix;              // World matrix
    Matrix4x4 worldInverseTranspose;    // World matrix inverse transpose(法線用)
};

struct alignas(16) MaterialConstants
{
    float4 baseColorFactor = { 1,1,1,1 };      // default=[1,1,1,1]
    float3 emissiveFactor = { 0,0,0 };         // default=[0,0,0]
    float normalTextureScale = 1.0f;           // default=1
    float metallicFactor = 0.0f;               // default=0
    float roughnessFactor = 0.5f;              // default=0.5
    union
    {
        uint flags;
        struct
        {
            //各テクスチャのUV
            uint baseColorUV : 1;
            uint metallicRoughnessUV : 1;
            uint occlusionUV : 1;
            uint emissiveUV : 1;
            uint normalUV : 1;

            //3つの特別モード
            uint twoSided : 1;
            uint alphaTest : 1;
            uint alphaBlend : 1;

            uint pad0 : 8;

            uint alphaRef : 16;// 半精度浮動小数
        };
    };
    float pad;
};

struct alignas(16) GlobalConstants
{
    float4x4 ViewProjMatrix;    // ビュー・プロジェクション行列
    float4x4 SunShadowMatrix;   // light space行列
    float3 CameraPos;           // カメラ位置
    float pad0;
    float3 SunDirection;        // メイン平行光源の方向
    float pad1;
    float3 SunIntensity;        // メイン平行光源の強度
    float pad2;
    float IBLRange;             // IBLの範囲
    float IBLBias;              // IBLのバイアス
    float2 pad3;           
};
