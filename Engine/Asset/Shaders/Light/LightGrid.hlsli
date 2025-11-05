#include "LightData.h"

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
