#include "GoalSystem.h"
#include "Runtime/Function/Framework/ECS/GameObject.h"
#include "Runtime/Function/Framework/Component/MeshComponent.h"
#include "Runtime/Function/Framework/Component/MaterialComponent.h"
#include "Runtime/Resource/AssetManager.h"
#include "Runtime/Core/Utility/Utility.h"
#include "SoundManaged.h"
#include "../Tag.h"
#include <cmath>
#include <fstream>
#include <filesystem>
#include <json.hpp>

using namespace AtomEngine;
using json = nlohmann::json;

void GoalSystem::Initialize(World& world, const std::string& configPath)
{
	mConfigPath = configPath;
	mGoalReached = false;

	if (std::filesystem::exists(configPath))
	{
		LoadFromJson(world, configPath);
	}
}

void GoalSystem::Update(World& world, float deltaTime)
{
	if (!mGoalReached)
	{
		CheckGoalCollision(world);
	}
}

void GoalSystem::CheckGoalCollision(World& world)
{
	auto& registry = world.GetRegistry();

	auto playerView = registry.view<TransformComponent, VoxelColliderComponent, PlayerTag>();

	auto goalView = registry.view<TransformComponent, GoalComponent, GoalTag>();

	for (auto playerEntity : playerView)
	{
		auto& playerTransform = playerView.get<TransformComponent>(playerEntity);
		auto& playerCollider = playerView.get<VoxelColliderComponent>(playerEntity);

		Vector3 playerPos = playerTransform.transition + playerCollider.offset;

		for (auto goalEntity : goalView)
		{
			auto& goalTransform = goalView.get<TransformComponent>(goalEntity);
			auto& goal = goalView.get<GoalComponent>(goalEntity);

			if (goal.isReached) continue;

			Vector3 goalPos = goalTransform.transition;

			Vector3 diff = playerPos - goalPos;
			float horizontalDistSqr = diff.x * diff.x + diff.z * diff.z;
			float verticalDist = std::abs(diff.y);

			if (horizontalDistSqr < goal.triggerRadius * goal.triggerRadius &&
				verticalDist < playerCollider.halfExtents.y + 2.0f)
			{
				goal.isReached = true;
				mGoalReached = true;

				Audio::GetInstance()->Play(Sound::gSoundMap["goal"]);
			}
		}
	}
}

Entity GoalSystem::CreateGoal(World& world, const GoalCreateInfo& info)
{
	auto goalObj = world.CreateGameObject(info.name);
	Entity entity = goalObj->GetHandle();

	world.AddComponent<TransformComponent>(entity, info.position, Quaternion::IDENTITY, 
		Vector3(info.scale, info.scale, info.scale));

	if (!info.modelPath.empty())
	{
		auto model = AssetManager::LoadModel(info.modelPath.c_str());
		if (model)
		{
			world.AddComponent<MaterialComponent>(entity, model);
			world.AddComponent<MeshComponent>(entity, model);
		}
	}

	GoalComponent goalComp;
	goalComp.triggerRadius = info.triggerRadius;
	world.AddComponent<GoalComponent>(entity, goalComp);

	world.AddComponent<GoalTag>(entity);

	return entity;
}

void GoalSystem::DestroyGoal(World& world, Entity entity)
{
	world.DestroyGameObject(entity);
}

bool GoalSystem::SaveToJson(World& world, const std::string& filePath)
{
	try
	{
		json root;
		root["goals"] = json::array();

		auto& registry = world.GetRegistry();
		auto view = registry.view<TransformComponent, GoalComponent>();

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& goal = view.get<GoalComponent>(entity);

			json goalJson;
			goalJson["position"] = { transform.transition.x, transform.transition.y, transform.transition.z };
			goalJson["scale"] = transform.scale.x;
			goalJson["triggerRadius"] = goal.triggerRadius;

			root["goals"].push_back(goalJson);
		}

		std::filesystem::path pathObj(filePath);
		std::filesystem::path dir = pathObj.parent_path();
		if (!dir.empty() && !std::filesystem::exists(dir))
		{
			std::filesystem::create_directories(dir);
		}

		std::ofstream file(filePath);
		if (!file.is_open()) return false;

		file << root.dump(4);
		file.close();

		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool GoalSystem::LoadFromJson(World& world, const std::string& filePath)
{
	try
	{
		std::ifstream file(filePath);
		if (!file.is_open()) return false;

		json root;
		file >> root;
		file.close();

		if (!root.contains("goals")) return false;

		for (const auto& goalJson : root["goals"])
		{
			GoalCreateInfo info;

			auto pos = goalJson["position"];
			info.position = Vector3(pos[0], pos[1], pos[2]);
			info.scale = goalJson.value("scale", 1.0f);
			info.triggerRadius = goalJson.value("triggerRadius", 3.0f);
			info.name = "Goal";

			CreateGoal(world, info);
		}

		return true;
	}
	catch (...)
	{
		return false;
	}
}
