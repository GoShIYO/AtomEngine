#pragma once
#include "Runtime/Function/Framework/ECS/World.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include "../Component/GoalComponent.h"
#include "../Component/VoxelColliderComponent.h"
#include <string>

using namespace AtomEngine;

class GoalSystem
{
public:
	GoalSystem() = default;

	void Initialize(World& world, const std::string& configPath = "Asset/Config/goals.json");

	void Update(World& world, float deltaTime);

	Entity CreateGoal(World& world, const GoalCreateInfo& info);

	void DestroyGoal(World& world, Entity entity);

	bool SaveToJson(World& world, const std::string& filePath);
	bool LoadFromJson(World& world, const std::string& filePath);

	bool IsGoalReached() const { return mGoalReached; }

	void Reset() { mGoalReached = false; }

private:
	void CheckGoalCollision(World& world);

private:
	bool mGoalReached{ false };
	std::string mConfigPath;
};
