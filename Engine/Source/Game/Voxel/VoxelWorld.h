#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include <array>
#undef min
#undef max

using namespace AtomEngine;

struct VoxelBound
{
    Vector3 min, max;
    VoxelBound() {}
    VoxelBound(const Vector3& a, const Vector3& b) : min(a), max(b) {}
    float Width() const { return max.x - min.x; }
    float Height() const { return max.y - min.y; }
    float Depth() const { return max.z - min.z; }
};
class VoxelWorld
{
public:
    int sizeX, sizeY, sizeZ;
    float voxelSize = 1.0f;
    std::vector<uint8_t> data; // 0 = empty : else = solid
    Vector3 worldSize;
    bool Load(const std::string& file);

    VoxelWorld(int sx, int sy, int sz, float vs = 1.0f)
        : sizeX(sx), sizeY(sy), sizeZ(sz), voxelSize(vs), data(sx* sy* sz, 0)
    {
    }
    VoxelWorld() = default;
    inline int Index(int x, int y, int z) const { return (z * sizeY + y) * sizeX + x; }
    bool InBounds(int x, int y, int z) const
    {
        return x >= 0 && x < sizeX && y >= 0 && y < sizeY && z >= 0 && z < sizeZ;
    }

    void SetSolid(int x, int y, int z, bool solid = true)
    {
        if (!InBounds(x, y, z)) return;
        data[Index(x, y, z)] = solid ? 1 : 0;
    }
    bool IsSolid(int x, int y, int z) const
    {
        if (!InBounds(x, y, z)) return false;
        return data[Index(x, y, z)] != 0;
    }

    int WorldToVoxelX(float wx) const;
    int WorldToVoxelY(float wy) const;
    int WorldToVoxelZ(float wz) const;

    VoxelBound VoxelAABB(int vx, int vy, int vz) const
    {
        float halfSize = voxelSize * 0.5f;
        Vector3 mn(vx - halfSize, vy - halfSize, vz - halfSize);
        Vector3 mx(vx + halfSize, vy + halfSize, vz + halfSize);
        return VoxelBound(mn, mx);
    }
};

inline int VoxelWorld::WorldToVoxelX(float wx) const { return static_cast<int>(std::floor(wx / voxelSize)); }
inline int VoxelWorld::WorldToVoxelY(float wy) const { return static_cast<int>(std::floor(wy / voxelSize)); }
inline int VoxelWorld::WorldToVoxelZ(float wz) const { return static_cast<int>(std::floor(wz / voxelSize)); }

inline bool AABBOverlap(const VoxelBound& a, const VoxelBound& b, Vector3& outPenetration)
{
    float overlapX = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    if (overlapX <= 0.0f) return false;
    float overlapY = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
    if (overlapY <= 0.0f) return false;
    float overlapZ = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);
    if (overlapZ <= 0.0f) return false;

    float aCenterX = (a.min.x + a.max.x) * 0.5f;
    float bCenterX = (b.min.x + b.max.x) * 0.5f;
    float aCenterY = (a.min.y + a.max.y) * 0.5f;
    float bCenterY = (b.min.y + b.max.y) * 0.5f;
    float aCenterZ = (a.min.z + a.max.z) * 0.5f;
    float bCenterZ = (b.min.z + b.max.z) * 0.5f;

    if (overlapX <= overlapY && overlapX <= overlapZ)
    {
        outPenetration = Vector3((aCenterX < bCenterX) ? -overlapX : overlapX, 0.0f, 0.0f);
    }
    else if (overlapY <= overlapX && overlapY <= overlapZ)
    {
        outPenetration = Vector3(0.0f, (aCenterY < bCenterY) ? -overlapY : overlapY, 0.0f);
    }
    else
    {
        outPenetration = Vector3(0.0f, 0.0f, (aCenterZ < bCenterZ) ? -overlapZ : overlapZ);
    }
    return true;
}

template<typename Callback>
inline void ForEachOverlappingVoxel(const VoxelWorld& world, const VoxelBound& box, Callback cb)
{
    int minX = world.WorldToVoxelX(box.min.x);
    int minY = world.WorldToVoxelY(box.min.y);
    int minZ = world.WorldToVoxelZ(box.min.z);

    const float eps = 1e-6f;
    int maxX = world.WorldToVoxelX(box.max.x - eps);
    int maxY = world.WorldToVoxelY(box.max.y - eps);
    int maxZ = world.WorldToVoxelZ(box.max.z - eps);

    for (int z = minZ; z <= maxZ; ++z)
    {
        for (int y = minY; y <= maxY; ++y)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                cb(x, y, z);
            }
        }
    }
}

struct Body
{
    Vector3 size;
    Vector3 velocity;
};

inline bool MoveAndResolveVoxelCollisions(VoxelWorld& world, Body& body,Vector3& position, float dt)
{
    Vector3 move = body.velocity * dt;

    const std::array<int, 3> axes = { 0,1,2 };
    for (int axis : axes)
    {
        Vector3 attemptedPos = position;
        if (axis == 0) attemptedPos.x += move.x;
        else if (axis == 1) attemptedPos.y += move.y;
        else attemptedPos.z += move.z;

        VoxelBound aabb(attemptedPos, attemptedPos + body.size);

        bool collided = false;
        Vector3 totalCorrection{ 0,0,0 };

        ForEachOverlappingVoxel(world, aabb, [&](int vx, int vy, int vz)
            {
                if (!world.InBounds(vx, vy, vz)) return;
                if (!world.IsSolid(vx, vy, vz)) return;
                VoxelBound voxelBox = world.VoxelAABB(vx, vy, vz);
                Vector3 pen;
                if (AABBOverlap(aabb, voxelBox, pen))
                {
                    collided = true;
                    if (axis == 0)
                    {
                        attemptedPos.x += pen.x;
                        totalCorrection.x += pen.x;
                    }
                    else if (axis == 1)
                    {
                        attemptedPos.y += pen.y;
                        totalCorrection.y += pen.y;
                    }
                    else
                    {
                        attemptedPos.z += pen.z;
                        totalCorrection.z += pen.z;
                    }
                    aabb.min = attemptedPos;
                    aabb.max = attemptedPos + body.size;
                }
            });

        position = attemptedPos;

        if (collided)
        {
            if (axis == 0) body.velocity.x = 0.0f;
            if (axis == 1) body.velocity.y = 0.0f;
            if (axis == 2) body.velocity.z = 0.0f;
        }
        return collided;
    }
    return false;
}