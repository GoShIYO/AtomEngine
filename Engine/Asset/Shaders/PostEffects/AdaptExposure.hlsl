#include "PostEffectUtil.hlsli"

ByteAddressBuffer Histogram : register(t0);
RWStructuredBuffer<float> Exposure : register(u0);

cbuffer cb0 : register(b1)
{
    float TargetLuminance;
    float AdaptationRate;
    float MinExposure;
    float MaxExposure;
    uint PixelCount;
}

groupshared float gAccum[256];

[numthreads(256, 1, 1)]
void main(uint GroupIndex : SV_GroupIndex)
{
    float WeightedSum = (float) GroupIndex * (float) Histogram.Load(GroupIndex * 4);

    [unroll]
    for (uint i = 1; i < 256; i *= 2)
    {
        gAccum[GroupIndex] = WeightedSum;               // Write
        GroupMemoryBarrierWithGroupSync();              // Sync
        WeightedSum += gAccum[(GroupIndex + i) % 256];  // Read
        GroupMemoryBarrierWithGroupSync();              // Sync
    }

    //画像全体が黒の場合は露出を調整しないでください
    if (WeightedSum == 0.0)
        return;

    float MinLog = Exposure[4];
    float MaxLog = Exposure[5];
    float LogRange = Exposure[6];
    float RcpLogRange = Exposure[7];

    // 平均ヒストグラム値は、すべてのピクセルの加重合計をピクセル総数で割った値
    // 加重を与えなかったピクセル（つまり黒ピクセル）を除いた値
    float weightedHistAvg = WeightedSum / (max(1, PixelCount - Histogram.Load(0))) - 1.0;
    float logAvgLuminance = exp2(weightedHistAvg / 254.0 * LogRange + MinLog);
    float targetExposure = TargetLuminance / logAvgLuminance;
    //float targetExposure = -log2(1 - TargetLuminance) / logAvgLuminance;

    float exposure = Exposure[0];
    exposure = lerp(exposure, targetExposure, AdaptationRate);
    exposure = clamp(exposure, MinExposure, MaxExposure);

    if (GroupIndex == 0)
    {
        Exposure[0] = exposure;
        Exposure[1] = 1.0 / exposure;
        Exposure[2] = exposure;
        Exposure[3] = weightedHistAvg;

        //ヒストグラムを対数平均を中心に再中心化する最初の試み
        float biasToCenter = (floor(weightedHistAvg) - 128.0) / 255.0;
        if (abs(biasToCenter) > 0.1)
        {
            MinLog += biasToCenter * RcpLogRange;
            MaxLog += biasToCenter * RcpLogRange;
        }

        //TODO: 値の範囲に合うように、対数範囲を増減します。
        //(アイデア) 過小または過大に代表される極端な境界については、
        //中間の対数加重合計を確認します。
        //つまり、for ループを2つに分割して、16 個のグループの合計を計算し、
        //両端のグループをチェックしてから、再帰合計を完了します。
        Exposure[4] = MinLog;
        Exposure[5] = MaxLog;
        Exposure[6] = LogRange;
        Exposure[7] = 1.0 / LogRange;
    }
}