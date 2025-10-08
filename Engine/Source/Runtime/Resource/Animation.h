#pragma once
#include <vector>
#include <cstdint>

namespace AtomEngine
{

    // アニメーションカーブは、値の時間経過に伴う変化を表します。
    // キーフレームはカーブを区切る役割を果たし、キーフレーム間の時間は
    // 選択された方法を使用して補間されます。カーブはアニメーション全体にわたって定義する必要はありません。
    // 一部のプロパティは、一部の時間のみアニメーション化される場合があります。
    struct AnimationCurve
    {
        enum { kTranslation, kRotation, kScale, kWeights };         // targetPath
        enum { kLinear, kStep, kCatmullRomSpline, kCubicSpline };   // 補間方法
        enum { kSNorm8, kUNorm8, kSNorm16, kUNorm16, kFloat };      // format

        uint32_t targetNode : 28;           // アニメーション化されるノード
        uint32_t targetPath : 2;            // アニメーション化される変換の側面
        uint32_t interpolation : 2;         // 補間方法
        uint32_t keyFrameOffset : 26;       // 最初のキーフレームへのバイトオフセット
        uint32_t keyFrameFormat : 3;        // キーフレームのデータ形式
        uint32_t keyFrameStride : 3;        // 1つのキーフレームあたりの4バイトワード数
        float numSegments;                  // キーフレーム間の均等間隔の分割数
        float startTime;                    // 最初のキーフレームのタイムスタンプ
        float rangeScale;                   // numSegments / (endTime - startTime)
    };

    // 複数のアニメーションカーブで構成
    struct AnimationSet
    {
        float duration;             // アニメーション全体の再生時間
        uint32_t firstCurve;        // このセットの最初のカーブのインデックス（別途保存されます）
        uint32_t numCurves;         // このセット内のカーブの数
    };

    // アニメーション状態は、アニメーションが再生中かどうかを示し、アニメーションの再生中の現在の位置を追跡します。
    struct AnimationState
    {
        enum eMode { kStopped, kPlaying, kLooping };
        eMode state;
        float time;
        AnimationState() : state(kStopped), time(0.0f) {}
    };
}
