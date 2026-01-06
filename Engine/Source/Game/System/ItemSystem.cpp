#include "ItemSystem.h"
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

void ItemSystem::Initialize(World& world, const std::string& configPath)
{
	mConfigPath = configPath;
	mCollectedCount = 0;
	mTotalCount = 0;

	if (std::filesystem::exists(configPath))
	{
		LoadFromJson(world, configPath);
	}
}

void ItemSystem::Update(World& world, float deltaTime)
{
	UpdateItemAnimations(world, deltaTime);
	CheckItemCollisions(world);
}

void ItemSystem::CheckItemCollisions(World& world)
{
	auto& registry = world.GetRegistry();

	auto playerView = registry.view<TransformComponent, VoxelColliderComponent, PlayerTag>();
	auto itemView = registry.view<TransformComponent, ItemComponent, ItemTag>();

	for (auto playerEntity : playerView)
	{
		auto& playerTransform = playerView.get<TransformComponent>(playerEntity);
		auto& playerCollider = playerView.get<VoxelColliderComponent>(playerEntity);

		Vector3 playerPos = playerTransform.transition + playerCollider.offset;
		Vector3 playerMin = playerPos - playerCollider.halfExtents;
		Vector3 playerMax = playerPos + playerCollider.halfExtents;

		for (auto itemEntity : itemView)
		{
			auto& itemTransform = itemView.get<TransformComponent>(itemEntity);
			auto& item = itemView.get<ItemComponent>(itemEntity);

			if (item.isPickedUp) continue;

			float itemRadius = itemTransform.scale.x * 0.5f;
			Vector3 itemPos = itemTransform.transition;
			Vector3 itemMin = itemPos - Vector3(itemRadius, itemRadius, itemRadius);
			Vector3 itemMax = itemPos + Vector3(itemRadius, itemRadius, itemRadius);

			if (AABBOverlap(playerMin, playerMax, itemMin, itemMax))
			{
				item.isPickedUp = true;
				mCollectedCount++;

				Audio::GetInstance()->Play(Sound::gSoundMap["pickup"]);
				DestroyItem(world, itemEntity);
			}
		}
	}
}

void ItemSystem::UpdateItemAnimations(World& world, float deltaTime)
{
	auto& registry = world.GetRegistry();
	auto view = registry.view<TransformComponent, ItemComponent>();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& item = view.get<ItemComponent>(entity);

		if (item.isPickedUp) continue;

		item.currentTime += deltaTime;

		float rotAngle = item.rotationSpeed * deltaTime;
		Quaternion rot = Quaternion::GetQuaternionFromAngleAxis(Radian(rotAngle), Vector3::UP);
		transform.rotation = transform.rotation * rot;
		transform.rotation.Normalize();

		float bobOffset = std::sin(item.currentTime * item.bobSpeed) * item.bobAmount;
		transform.transition.y = item.baseY + bobOffset;
		
		transform.scale = item.baseScale + bobOffset;
	}
}

Entity ItemSystem::CreateItem(World& world, const ItemCreateInfo& info)
{
	auto itemObj = world.CreateGameObject(info.name);
	Entity entity = itemObj->GetHandle();

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

	ItemComponent itemComp;
	itemComp.itemId = mNextItemId++;
	itemComp.baseY = info.position.y;
	itemComp.baseScale = info.scale;
	world.AddComponent<ItemComponent>(entity, itemComp);

	world.AddComponent<ItemTag>(entity);

	mTotalCount++;

	if (mAddGameObjectCallback)
	{
		mAddGameObjectCallback(std::move(itemObj));
	}

	return entity;
}

void ItemSystem::DestroyItem(World& world, Entity entity)
{
	auto& registry = world.GetRegistry();
	if (!registry.any_of<DeathFlag>(entity))
	{
		registry.emplace<DeathFlag>(entity, DeathFlag{ true });
	}
	else
	{
		registry.get<DeathFlag>(entity).isDead = true;
	}
}

bool ItemSystem::AABBOverlap(const Vector3& min1, const Vector3& max1,
	const Vector3& min2, const Vector3& max2) const
{
	if (max1.x < min2.x || min1.x > max2.x) return false;
	if (max1.y < min2.y || min1.y > max2.y) return false;
	if (max1.z < min2.z || min1.z > max2.z) return false;
	return true;
}

bool ItemSystem::SaveToJson(World& world, const std::string& filePath)
{
	try
	{
		json root;
		root["items"] = json::array();

		auto& registry = world.GetRegistry();
		auto view = registry.view<TransformComponent, ItemComponent>();

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& item = view.get<ItemComponent>(entity);

			if (item.isPickedUp) continue;

			json itemJson;
			itemJson["id"] = item.itemId;
			itemJson["position"] = { transform.transition.x, item.baseY, transform.transition.z };
			itemJson["scale"] = transform.scale.x;
			itemJson["rotationSpeed"] = item.rotationSpeed;
			itemJson["bobSpeed"] = item.bobSpeed;
			itemJson["bobAmount"] = item.bobAmount;

			root["items"].push_back(itemJson);
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

bool ItemSystem::LoadFromJson(World& world, const std::string& filePath)
{
	try
	{
		std::ifstream file(filePath);
		if (!file.is_open()) return false;

		json root;
		file >> root;
		file.close();

		if (!root.contains("items")) return false;

		for (const auto& itemJson : root["items"])
		{
			ItemCreateInfo info;
			
			auto pos = itemJson["position"];
			info.position = Vector3(pos[0], pos[1], pos[2]);
			info.scale = itemJson.value("scale", 1.0f);
			info.name = "Item_" + std::to_string(itemJson.value("id", 0));

			if (itemJson.contains("modelPath"))
			{
				info.modelPath = UTF8ToWString(itemJson["modelPath"].get<std::string>());
			}

			Entity entity = CreateItem(world, info);

			if (world.GetRegistry().valid(entity))
			{
				auto& item = world.GetComponent<ItemComponent>(entity);
				item.rotationSpeed = itemJson.value("rotationSpeed", 2.0f);
				item.bobSpeed = itemJson.value("bobSpeed", 3.0f);
				item.bobAmount = itemJson.value("bobAmount", 0.3f);
			}
		}

		return true;
	}
	catch (...)
	{
		return false;
	}
}
