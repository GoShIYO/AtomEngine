#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include <array>
#include <optional>
#include <vector>
#undef min
#undef max

using namespace AtomEngine;

struct VoxelHit
{
    Vector3 position{};   // contact point in world space
    Vector3 normal{};  // separating normal
    float distance{0.0f}; // distance from ray origin
    int voxelX{0}, voxelY{0}, voxelZ{0}; // voxel coordinates
};

struct VoxelSweepResult
{
    bool hit{false};
    float time{1.0f};
    Vector3 normal{};
    Vector3 contactPoint{};
    int voxelX{0}, voxelY{0}, voxelZ{0};
};

struct VoxelCollisionResult
{
    bool collided{false};
    Vector3 penetration{};
    Vector3 normal{};
    Vector3 correctedPosition{};
};

class VoxelWorld
{
public:
    struct CellCoord
    {
        int x{ 0 }, y{ 0 }, z{ 0 };
    };

    int width{ 0 }, height{ 0 }, depth{ 0 };
    float voxelSize = 1.0f;
    Vector3 origin = Vector3::ZERO;
    std::vector<uint8_t> data; // 0 = empty : else = solid
    Vector3 worldSize = Vector3::ZERO; // in world units
    bool Load(const std::string& file);

    VoxelWorld() = default;

    void Resize(int w,int h,int d);

    inline int Index(int x, int y, int z) const { return x + y * width + z * width * height; }
    bool InBounds(int x, int y, int z) const
    {
        return x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < depth;
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

    void Set(int x, int y, int z, uint8_t value)
    {
        if (!InBounds(x,y,z))return;
        data[Index(x,y,z)] = value;
    }

    uint8_t Get(int x, int y, int z) const
    {
        if (!InBounds(x,y,z))return 0;
        return data[Index(x,y,z)];
    }

    std::optional<CellCoord> WorldToCoord(const Vector3& worldPos) const;
    Vector3 CoordToWorldCenter(const CellCoord& coord) const;
    Vector3 CoordToWorldMin(int x, int y, int z) const;
    Vector3 CoordToWorldMax(int x, int y, int z) const;

    bool IsSolid(const Vector3& worldPos) const;
    bool OverlapsSolid(const Vector3& worldMin, const Vector3& worldMax) const;

    std::optional<VoxelHit> Raycast(const Vector3& origin, const Vector3& direction, float maxDistance = 1000.0f) const;
    
    VoxelSweepResult SweepAABB(const Vector3& boxMin, const Vector3& boxMax, 
                const Vector3& velocity) const;
    
    VoxelCollisionResult ResolveAABBCollision(const Vector3& boxMin, const Vector3& boxMax) const;
    
    std::vector<CellCoord> GetOverlappingVoxels(const Vector3& boxMin, const Vector3& boxMax) const;
    
    Vector3 MoveAndSlide(const Vector3& position, const Vector3& halfExtents, 
      const Vector3& velocity, int maxIterations = 4) const;
    
    bool IsGrounded(const Vector3& position, const Vector3& halfExtents, float groundCheckDistance = 0.1f) const;

private:
    float SignedDistanceToVoxel(const Vector3& point, int vx, int vy, int vz) const;
    Vector3 GetVoxelNormal(const Vector3& point, int vx, int vy, int vz) const;
};

struct VoxelWorldComponent
{
    VoxelWorld world;
};
