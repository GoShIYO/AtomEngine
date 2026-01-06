#pragma once
#include "Runtime/Function/Framework/ECS/World.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include "../Component/LadderComponent.h"
#include "../Component/VoxelColliderComponent.h"
#include <string>

using namespace AtomEngine;

// 玩家攀爬状态组件 - 添加到正在攀爬的实体上
struct ClimbingStateComponent
{
    Entity currentLadder{ entt::null };  // 当前攀爬的梯子
    Vector3 ladderTop{ 0, 0, 0 };        // 梯子顶部位置
    Vector3 ladderCenter{ 0, 0, 0 };     // 梯子中心位置
    Vector3 climbDirection{ 0, 1, 0 };   // 攀爬方向
    float climbSpeed{ 5.0f };   // 攀爬速度
    bool reachedTop{ false };            // 是否到达顶部
};

class LadderSystem
{
public:
    LadderSystem() = default;

    // 初始化系统，尝试从配置文件加载
    void Initialize(World& world, const std::string& configPath = "Asset/Config/ladders.json");

 // 每帧更新
    void Update(World& world, float deltaTime);

    // 创建梯子
    Entity CreateLadder(World& world, const LadderCreateInfo& info);

    // 销毁梯子
    void DestroyLadder(World& world, Entity entity);

    // 检查实体是否正在攀爬
    bool IsClimbing(World& world, Entity entity) const;

    // 保存梯子配置到JSON
    bool SaveToJson(World& world, const std::string& filePath);

    // 从JSON加载梯子配置
    bool LoadFromJson(World& world, const std::string& filePath);

    // 获取下一个梯子ID
    int GetNextLadderId() { return mNextLadderId++; }

    // 获取配置文件路径
    const std::string& GetConfigPath() const { return mConfigPath; }

private:
    // 检测玩家与梯子的碰撞
    void CheckLadderCollisions(World& world);

    // 处理攀爬中的实体
    void ProcessClimbing(World& world, float deltaTime);

    // 开始攀爬
    void StartClimbing(World& world, Entity climber, Entity ladder);

    // 结束攀爬
    void StopClimbing(World& world, Entity climber);

    // AABB重叠检测
    bool AABBOverlap(const Vector3& min1, const Vector3& max1,
  const Vector3& min2, const Vector3& max2) const;

private:
    int mNextLadderId{ 1 };
    std::string mConfigPath;
};
