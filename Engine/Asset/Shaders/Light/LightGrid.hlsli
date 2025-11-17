#define MAX_LIGHTS 128
#define TILE_SIZE (4 + MAX_LIGHTS * 4)

struct LightData
{
    float3 position;
    float radiusSq;
    float3 color;

    uint type;
    float3 direction;
    float innerCos;
    float outerCos;

    float4x4 shadowMatrix;
};

uint2 GetTilePos(float2 pos, float2 invTileDim)
{
    return pos * invTileDim;
}
uint GetTileIndex(uint2 tilePos, uint tileCountX)
{
    return tilePos.y * tileCountX + tilePos.x;
}
uint GetTileOffset(uint tileIndex)
{
    return tileIndex * TILE_SIZE;
}
