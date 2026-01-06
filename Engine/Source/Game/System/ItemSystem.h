#pragma once
#include "Runtime/Function/Framework/ECS/World.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include "Runtime/Function/Framework/ECS/GameObject.h"
#include "../Component/ItemComponent.h"
#include "../Component/VoxelColliderComponent.h"
#include <string>
#include <unordered_map>
#include <functional>

using namespace AtomEngine;

struct ItemCollectedEvent
{
	Entity item;
	Entity collector;
	int itemId;
};

using AddGameObjectCallback = std::function<void(std::unique_ptr<GameObject>&&)>;

class ItemSystem
{
public:
	ItemSystem() = default;
	~ItemSystem() = default;

	void SetAddGameObjectCallback(AddGameObjectCallback callback) { mAddGameObjectCallback = callback; }

	void Initialize(World& world, const std::string& configPath = "Asset/Config/items.json");

	void Update(World& world, float deltaTime);

	Entity CreateItem(World& world, const ItemCreateInfo& info);

	void DestroyItem(World& world, Entity entity);

	bool SaveToJson(World& world, const std::string& filePath);
	bool LoadFromJson(World& world, const std::string& filePath);

	int GetCollectedCount() const { return mCollectedCount; }

	int GetTotalCount() const { return mTotalCount; }

	void ResetCount() { mCollectedCount = 0; mTotalCount = 0; }

	int GetNextItemId() { return mNextItemId++; }

private:
	void CheckItemCollisions(World& world);

	void UpdateItemAnimations(World& world, float deltaTime);

	bool AABBOverlap(const Vector3& min1, const Vector3& max1,
		const Vector3& min2, const Vector3& max2) const;

private:
	int mNextItemId{ 1 };
	int mCollectedCount{ 0 };
	int mTotalCount{ 0 };
	std::string mConfigPath;
	AddGameObjectCallback mAddGameObjectCallback;
};
