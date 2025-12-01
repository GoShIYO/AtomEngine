Texture2D<float3> BloomBuf : register(t0);
RWTexture2D<float3> Result1 : register(u0);
RWTexture2D<float3> Result2 : register(u1);
RWTexture2D<float3> Result3 : register(u2);
RWTexture2D<float3> Result4 : register(u3);
SamplerState BiLinearClamp : register(s0);

cbuffer cb0 : register(b0)
{
    float2 ginverseDimensions;
}

groupshared float3 gTile[64]; //8x8入力ピクセル

[numthreads(8, 8, 1)]
void main(uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID)
{
    //この値でxとyが2の累乗で割り切れるかどうかが分かります
    uint parity = DTid.x | DTid.y;

    //8x8ブロックをダウンサンプリングして保存
    float2 centerUV = (float2(DTid.xy) * 2.0f + 1.0f) * ginverseDimensions;
    float3 avgPixel = BloomBuf.SampleLevel(BiLinearClamp, centerUV, 0.0f);
    gTile[GI] = avgPixel;
    Result1[DTid.xy] = avgPixel;

    GroupMemoryBarrierWithGroupSync();

    //4x4ブロックをダウンサンプリング
    if ((parity & 1) == 0)
    {
        avgPixel = 0.25f * (avgPixel + gTile[GI + 1] + gTile[GI + 8] + gTile[GI + 9]);
        gTile[GI] = avgPixel;
        Result2[DTid.xy >> 1] = avgPixel;
    }

    GroupMemoryBarrierWithGroupSync();

    //2x2ブロックをダウンサンプリング
    if ((parity & 3) == 0)
    {
        avgPixel = 0.25f * (avgPixel + gTile[GI + 2] + gTile[GI + 16] + gTile[GI + 18]);
        gTile[GI] = avgPixel;
        Result3[DTid.xy >> 2] = avgPixel;
    }

    GroupMemoryBarrierWithGroupSync();

    //1x1ブロックをダウンサンプリング
    if ((parity & 7) == 0)
    {
        avgPixel = 0.25f * (avgPixel + gTile[GI + 4] + gTile[GI + 32] + gTile[GI + 36]);
        Result4[DTid.xy >> 3] = avgPixel;
    }
}