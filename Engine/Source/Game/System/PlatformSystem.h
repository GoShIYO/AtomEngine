#include "Runtime/Function/Framework/ECS/World.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include "Runtime/Function/Framework/ECS/GameObject.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "../Component/PlatformComponent.h"
#include "../Component/DynamicVoxelBodyComponent.h"
#include <string>

using namespace AtomEngine;

class PlatformSystem
{
public:
	PlatformSystem() = default;

	void Initialize(World& world, const std::string& configPath = "Asset/Config/platforms.json");

	void Update(World& world, float deltaTime);

	Entity CreatePlatform(World& world, const PlatformCreateInfo& info);

	void DestroyPlatform(World& world, Entity entity);

	bool SaveToJson(World& world, const std::string& filePath);

	bool LoadFromJson(World& world, const std::string& filePath);

	int GetNextPlatformId() { return mNextPlatformId++; }

	const std::string& GetConfigPath() const { return mConfigPath; }

private:
	void UpdateLinearMotion(PlatformComponent& platform, TransformComponent& transform, float deltaTime);

	void UpdateCircularMotion(PlatformComponent& platform, TransformComponent& transform, float deltaTime);

	void UpdateSelfRotation(PlatformComponent& platform, TransformComponent& transform, float deltaTime);

private:
	int mNextPlatformId{ 0 };
	std::string mConfigPath;
};
