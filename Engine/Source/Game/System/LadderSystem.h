#pragma once
#include "Runtime/Function/Framework/ECS/World.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include "../Component/LadderComponent.h"
#include "../Component/VoxelColliderComponent.h"
#include <string>

using namespace AtomEngine;

struct ClimbingStateComponent
{
    Entity currentLadder{ entt::null };
    Vector3 ladderTop{ 0, 0, 0 };
    Vector3 ladderCenter{ 0, 0, 0 };
    Vector3 climbDirection{ 0, 1, 0 }; 
    float climbSpeed{ 5.0f };
    bool reachedTop{ false };
};

class LadderSystem
{
public:
    LadderSystem() = default;

    void Initialize(World& world, const std::string& configPath = "Asset/Config/ladders.json");

    void Update(World& world, float deltaTime);

    Entity CreateLadder(World& world, const LadderCreateInfo& info);

    void DestroyLadder(World& world, Entity entity);

    bool IsClimbing(World& world, Entity entity) const;

    bool SaveToJson(World& world, const std::string& filePath);

    bool LoadFromJson(World& world, const std::string& filePath);

    int GetNextLadderId() { return mNextLadderId++; }

    const std::string& GetConfigPath() const { return mConfigPath; }

private:
    void CheckLadderCollisions(World& world);

    void ProcessClimbing(World& world, float deltaTime);

    void StartClimbing(World& world, Entity climber, Entity ladder);

    void StopClimbing(World& world, Entity climber);

    bool AABBOverlap(const Vector3& min1, const Vector3& max1,
  const Vector3& min2, const Vector3& max2) const;

private:
    int mNextLadderId{ 1 };
    std::string mConfigPath;
};
