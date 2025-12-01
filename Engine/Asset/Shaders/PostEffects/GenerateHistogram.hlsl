
// グループサイズは16x16ですが、1つのグループは幅16のピクセル列全体（高さ384ピクセル）を反復処理します。
// ワークスペース全体が640x384であると仮定すると、40のスレッドグループがヒストグラムを並列計算することになります。
// ヒストグラムは、2^-12から2^4の範囲の対数輝度を測定します。これにより、露出が2^-4から2^4の範囲になる適切なウィンドウが提供されます。

Texture2D<uint> LumaBuf : register(t0);
RWByteAddressBuffer Histogram : register(u0);

groupshared uint gTileHistogram[256];

cbuffer CB0 : register(b0)
{
    uint kBufferHeight;
}

[numthreads(16, 16, 1)]
void main(uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID)
{
    gTileHistogram[GI] = 0;

    GroupMemoryBarrierWithGroupSync();

    //全体が処理されるまでループ
    for (uint2 ST = DTid.xy; ST.y < kBufferHeight; ST.y += 16)
    {
        uint QuantizedLogLuma = LumaBuf[ST];
        InterlockedAdd(gTileHistogram[QuantizedLogLuma], 1);
    }

    GroupMemoryBarrierWithGroupSync();

    Histogram.InterlockedAdd(GI * 4, gTileHistogram[GI]);
}