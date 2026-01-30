#pragma once
#include <random>
#include <limits>

class Random
{
public:
    Random() = default;
    ~Random() = default;
    // intのデフォルト範囲は [MIN_INT, MAX_INT] です。最大値を含みます。
    int32_t NextInt()
    {
        return std::uniform_int_distribution<int32_t>(
            std::numeric_limits<int32_t>::min(),
            std::numeric_limits<int32_t>::max()
        )(mGen);
    }

    // 範囲は [0, MaxVal] です。最大値を含みます。
    int32_t NextInt(int32_t MaxVal)
    {
        return std::uniform_int_distribution<int32_t>(0, MaxVal)(mGen);
    }

    // 範囲は [MinVal, MaxVal] です。最大値を含みます。
    int32_t NextInt(int32_t MinVal, int32_t MaxVal)
    {
        return std::uniform_int_distribution<int32_t>(MinVal, MaxVal)(mGen);
    }

    // floatのデフォルト範囲は [0.0f, 1.0f) です。最大値は含まれません（未満）。
    float NextFloat(float MaxVal = 1.0f)
    {
        return std::uniform_real_distribution<float>(0.0f, MaxVal)(mGen);
    }

    // 範囲は [MinVal, MaxVal) です。最大値は含まれません（未満）。
    float NextFloat(float MinVal, float MaxVal)
    {
        return std::uniform_real_distribution<float>(MinVal, MaxVal)(mGen);
    }

    // シード値を手動で設定します
    void SetSeed(uint32_t s)
    {
        mGen.seed(s);
    }

private:
    std::random_device mRd;

    std::mt19937 mGen;
};