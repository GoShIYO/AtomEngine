#include "LadderSystem.h"
#include "Runtime/Function/Framework/ECS/GameObject.h"
#include "../Component/VelocityComponent.h"
#include "../Tag.h"
#include <cmath>
#include <fstream>
#include <filesystem>
#include <json.hpp>

using namespace AtomEngine;
using json = nlohmann::json;

void LadderSystem::Initialize(World& world, const std::string& configPath)
{
	mConfigPath = configPath;
	
	if (std::filesystem::exists(configPath))
	{
		LoadFromJson(world, configPath);
	}
}

void LadderSystem::Update(World& world, float deltaTime)
{
	CheckLadderCollisions(world);

	ProcessClimbing(world, deltaTime);
}

void LadderSystem::CheckLadderCollisions(World& world)
{
	auto& registry = world.GetRegistry();

	auto ladderView = registry.view<TransformComponent, LadderComponent>();

	auto climberView = registry.view<TransformComponent, VoxelColliderComponent, PlayerTag>();

	for (auto climberEntity : climberView)
	{
		if (registry.any_of<ClimbingStateComponent>(climberEntity))
		{
			continue;
		}

		auto& climberTransform = climberView.get<TransformComponent>(climberEntity);
		auto& climberCollider = climberView.get<VoxelColliderComponent>(climberEntity);

		Vector3 climberPos = climberTransform.transition + climberCollider.offset;
		Vector3 climberMin = climberPos - climberCollider.halfExtents;
		Vector3 climberMax = climberPos + climberCollider.halfExtents;

		for (auto ladderEntity : ladderView)
		{
			auto& ladderTransform = ladderView.get<TransformComponent>(ladderEntity);
			auto& ladder = ladderView.get<LadderComponent>(ladderEntity);

			if (!ladder.isEnabled) continue;

			Vector3 ladderPos = ladderTransform.transition;
			Vector3 ladderMin = ladderPos - ladder.halfExtents;
			Vector3 ladderMax = ladderPos + ladder.halfExtents;

			if (AABBOverlap(climberMin, climberMax, ladderMin, ladderMax))
			{
				StartClimbing(world, climberEntity, ladderEntity);
				break;
			}
		}
	}
}

void LadderSystem::ProcessClimbing(World& world, float deltaTime)
{
	auto& registry = world.GetRegistry();
	auto view = registry.view<TransformComponent, VoxelColliderComponent, ClimbingStateComponent>();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& collider = view.get<VoxelColliderComponent>(entity);
		auto& climbing = view.get<ClimbingStateComponent>(entity);

		if (!registry.valid(climbing.currentLadder))
		{
			StopClimbing(world, entity);
			continue;
		}

		auto& ladder = registry.get<LadderComponent>(climbing.currentLadder);
		auto& ladderTransform = registry.get<TransformComponent>(climbing.currentLadder);

		Vector3 ladderPos = ladderTransform.transition;
		climbing.ladderCenter = ladderPos;
		climbing.ladderTop = ladderPos + Vector3(0, ladder.halfExtents.y, 0);

		Vector3 currentPos = transform.transition + collider.offset;
		float currentFeetY = currentPos.y - collider.halfExtents.y;
		float ladderTopY = climbing.ladderTop.y;

		if (currentFeetY >= ladderTopY - 0.1f)
		{
			Vector3 exitPos = climbing.ladderTop;
			exitPos.y += collider.halfExtents.y + ladder.topExitUpOffset;

			Vector3 forward = Vector3(0, 0, 1);
			exitPos += forward * ladder.topExitForwardOffset;

			transform.transition = exitPos - collider.offset;

			StopClimbing(world, entity);
			continue;
		}

		float climbDistance = climbing.climbSpeed * deltaTime;
		transform.transition.y += climbDistance;

		Vector3 ladderCenter2D = climbing.ladderCenter;
		ladderCenter2D.y = transform.transition.y;

		Vector3 currentPos2D = transform.transition;
		currentPos2D.y = 0;
		ladderCenter2D.y = 0;

		Vector3 toCenter = climbing.ladderCenter - transform.transition;
		toCenter.y = 0;

		if (toCenter.LengthSqr() > 0.01f)
		{
			float centeringSpeed = 3.0f * deltaTime;
			transform.transition.x += toCenter.x * centeringSpeed;
			transform.transition.z += toCenter.z * centeringSpeed;
		}

		collider.verticalVelocity = 0.0f;
		collider.isGrounded = true;

		if (world.HasComponent<VelocityComponent>(entity))
		{
			auto& velocity = world.GetComponent<VelocityComponent>(entity);
			velocity.velocity = Vector3::ZERO;
		}
	}
}

void LadderSystem::StartClimbing(World& world, Entity climber, Entity ladder)
{
	auto& registry = world.GetRegistry();

	if (registry.any_of<ClimbingStateComponent>(climber))
	{
		return;
	}

	auto& ladderComp = registry.get<LadderComponent>(ladder);
	auto& ladderTransform = registry.get<TransformComponent>(ladder);

	Vector3 ladderPos = ladderTransform.transition;

	ClimbingStateComponent climbing;
	climbing.currentLadder = ladder;
	climbing.ladderCenter = ladderPos;
	climbing.ladderTop = ladderPos + Vector3(0, ladderComp.halfExtents.y, 0);
	climbing.climbDirection = Vector3(0, 1, 0);
	climbing.climbSpeed = ladderComp.climbSpeed;
	climbing.reachedTop = false;

	registry.emplace<ClimbingStateComponent>(climber, climbing);

	if (registry.any_of<VelocityComponent>(climber))
	{
		auto& velocity = registry.get<VelocityComponent>(climber);
		velocity.velocity = Vector3::ZERO;
	}
	
	if (registry.any_of<VoxelColliderComponent>(climber))
	{
		auto& collider = registry.get<VoxelColliderComponent>(climber);
		collider.verticalVelocity = 0.0f;
	}

	world.GetDispatcher().trigger<LadderClimbEvent>({
		ladder, climber, true
	});
}

void LadderSystem::StopClimbing(World& world, Entity climber)
{
	auto& registry = world.GetRegistry();

	if (!registry.any_of<ClimbingStateComponent>(climber))
	{
		return;
	}

	auto& climbing = registry.get<ClimbingStateComponent>(climber);
	Entity ladder = climbing.currentLadder;

	if (registry.any_of<VoxelColliderComponent>(climber))
	{
		auto& collider = registry.get<VoxelColliderComponent>(climber);
		collider.isGrounded = false;
		collider.verticalVelocity = 0.0f;
	}

	registry.remove<ClimbingStateComponent>(climber);

	world.GetDispatcher().trigger<LadderClimbEvent>({
		ladder, climber, false
	});
}

bool LadderSystem::IsClimbing(World& world, Entity entity) const
{
	return world.GetRegistry().any_of<ClimbingStateComponent>(entity);
}

Entity LadderSystem::CreateLadder(World& world, const LadderCreateInfo& info)
{
	auto ladderObj = world.CreateGameObject(info.name);
	Entity entity = ladderObj->GetHandle();

	world.AddComponent<TransformComponent>(entity, info.position, Quaternion::IDENTITY, Vector3::UNIT_SCALE);

	LadderComponent ladderComp;
	ladderComp.halfExtents = info.halfExtents;
	ladderComp.climbSpeed = info.climbSpeed;
	ladderComp.ladderName = info.name;
	ladderComp.ladderId = mNextLadderId++;
	world.AddComponent<LadderComponent>(entity, ladderComp);

	world.AddComponent<LadderTag>(entity);

	return entity;
}

void LadderSystem::DestroyLadder(World& world, Entity entity)
{
	world.DestroyGameObject(entity);
}

bool LadderSystem::AABBOverlap(const Vector3& min1, const Vector3& max1,
	const Vector3& min2, const Vector3& max2) const
{
	if (max1.x < min2.x || min1.x > max2.x) return false;
	if (max1.y < min2.y || min1.y > max2.y) return false;
	if (max1.z < min2.z || min1.z > max2.z) return false;
	return true;
}

bool LadderSystem::SaveToJson(World& world, const std::string& filePath)
{
	try
	{
		json root;
		root["ladders"] = json::array();

		auto& registry = world.GetRegistry();
		auto view = registry.view<TransformComponent, LadderComponent>();

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& ladder = view.get<LadderComponent>(entity);

			json ladderJson;
			ladderJson["name"] = ladder.ladderName;
			ladderJson["id"] = ladder.ladderId;
			ladderJson["position"] = { transform.transition.x, transform.transition.y, transform.transition.z };
			ladderJson["halfExtents"] = { ladder.halfExtents.x, ladder.halfExtents.y, ladder.halfExtents.z };
			ladderJson["climbSpeed"] = ladder.climbSpeed;
			ladderJson["topExitForwardOffset"] = ladder.topExitForwardOffset;
			ladderJson["topExitUpOffset"] = ladder.topExitUpOffset;
			ladderJson["isEnabled"] = ladder.isEnabled;

			root["ladders"].push_back(ladderJson);
		}

		std::filesystem::path pathObj(filePath);
		std::filesystem::path dir = pathObj.parent_path();
		if (!dir.empty() && !std::filesystem::exists(dir))
		{
			std::filesystem::create_directories(dir);
		}

		std::ofstream file(filePath);
		if (!file.is_open())
		{
			return false;
		}

		file << root.dump(4);
		file.close();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool LadderSystem::LoadFromJson(World& world, const std::string& filePath)
{
	try
	{
		std::ifstream file(filePath);
		if (!file.is_open())
		{
			return false;
		}

		json root;
		file >> root;
		file.close();

		if (!root.contains("ladders"))
		{
			return false;
		}

		auto& registry = world.GetRegistry();

		for (const auto& ladderJson : root["ladders"])
		{
			LadderCreateInfo info;
			info.name = ladderJson.value("name", "Ladder");

			auto pos = ladderJson["position"];
			info.position = Vector3(pos[0], pos[1], pos[2]);

			auto size = ladderJson["halfExtents"];
			info.halfExtents = Vector3(size[0], size[1], size[2]);

			info.climbSpeed = ladderJson.value("climbSpeed", 5.0f);

			Entity entity = CreateLadder(world, info);

			if (registry.valid(entity))
			{
				auto& ladder = registry.get<LadderComponent>(entity);
				ladder.topExitForwardOffset = ladderJson.value("topExitForwardOffset", 0.5f);
				ladder.topExitUpOffset = ladderJson.value("topExitUpOffset", 0.1f);
				ladder.isEnabled = ladderJson.value("isEnabled", true);
			}
		}

		return true;
	}
	catch (...)
	{
		return false;
	}
}
