#include "PlatformSystem.h"
#include "Runtime/Function/Framework/ECS/GameObject.h"
#include "Runtime/Function/Framework/Component/MeshComponent.h"
#include "Runtime/Function/Framework/Component/MaterialComponent.h"
#include "Runtime/Resource/AssetManager.h"
#include "Runtime/Core/Utility/Utility.h"
#include "../Component/VelocityComponent.h"
#include <cmath>
#include <fstream>
#include <filesystem>
#include <json.hpp>

using namespace AtomEngine;
using json = nlohmann::json;

// JSON辅助函数
static json Vec3ToJson(const Vector3& v)
{
	return json::array({ v.x, v.y, v.z });
}

static Vector3 JsonToVec3(const json& j)
{
	if (j.is_array() && j.size() >= 3)
		return Vector3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
	return Vector3::ZERO;
}

static json QuatToJson(const Quaternion& q)
{
	return json::array({ q.x, q.y, q.z, q.w });
}

static Quaternion JsonToQuat(const json& j)
{
	if (j.is_array() && j.size() >= 4)
		return Quaternion(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>());
	return Quaternion::IDENTITY;
}

void PlatformSystem::Initialize(World& world, const std::string& configPath)
{
	mConfigPath = configPath;
	
	if (std::filesystem::exists(configPath))
	{
		LoadFromJson(world, configPath);
	}
}

void PlatformSystem::Update(World& world, float deltaTime)
{
	auto& registry = world.GetRegistry();
	auto view = registry.view<TransformComponent, PlatformComponent, DynamicVoxelBodyComponent>();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& platform = view.get<PlatformComponent>(entity);
		auto& body = view.get<DynamicVoxelBodyComponent>(entity);

		if (!platform.isMoving) continue;

		Vector3 previousPos = transform.transition;
		Quaternion previousRot = transform.rotation;

		switch (platform.motionType)
		{
		case PlatformMotionType::Linear:
			UpdateLinearMotion(platform, transform, deltaTime);
			break;
		case PlatformMotionType::Circular:
			UpdateCircularMotion(platform, transform, deltaTime);
			break;
		case PlatformMotionType::Static:
		default:
			break;
		}

		if (platform.enableRotation)
		{
			UpdateSelfRotation(platform, transform, deltaTime);
		}

		if (deltaTime > 0.0001f)
		{
			body.velocity = (transform.transition - previousPos) / deltaTime;
			if (platform.enableRotation)
			{
				Quaternion rotDiff = transform.rotation * previousRot.Inverse();
				rotDiff.Normalize();

				float angle = 2.0f * std::acos(std::clamp(rotDiff.w, -1.0f, 1.0f));
				if (angle > 0.0001f)
				{
					float sinHalfAngle = std::sqrt(1.0f - rotDiff.w * rotDiff.w);
					if (sinHalfAngle > 0.0001f)
					{
						Vector3 axis(rotDiff.x / sinHalfAngle,
							rotDiff.y / sinHalfAngle,
							rotDiff.z / sinHalfAngle);
						body.angularVelocity = axis * (angle / deltaTime);
					}
					else
					{
						body.angularVelocity = Vector3::ZERO;
					}
				}
				else
				{
					body.angularVelocity = Vector3::ZERO;
				}
			}
			else
			{
				body.angularVelocity = Vector3::ZERO;
			}
		}

		body.previousRotation = previousRot;
		body.rotation = transform.rotation;
	}
}

void PlatformSystem::UpdateLinearMotion(PlatformComponent& platform, TransformComponent& transform, float deltaTime)
{
	Vector3 direction = platform.endPosition - platform.startPosition;
	float totalDistance = direction.Length();

	if (totalDistance < 0.001f) return;

	direction.Normalize();

	float moveDistance = platform.moveSpeed * deltaTime;
	platform.currentTime += moveDistance * platform.direction;

	if (platform.pingPong)
	{
		if (platform.currentTime >= totalDistance)
		{
			platform.currentTime = totalDistance;
			platform.direction = -1.0f;
		}
		else if (platform.currentTime <= 0.0f)
		{
			platform.currentTime = 0.0f;
			platform.direction = 1.0f;
		}
	}
	else
	{
		if (platform.currentTime >= totalDistance)
		{
			platform.currentTime = 0.0f;
		}
	}

	transform.transition = platform.startPosition + direction * platform.currentTime;
}

void PlatformSystem::UpdateCircularMotion(PlatformComponent& platform, TransformComponent& transform, float deltaTime)
{
	platform.currentTime += deltaTime * platform.rotationSpeed;

	const float twoPi = 6.28318530718f;
	if (platform.currentTime > twoPi)
	{
		platform.currentTime -= twoPi;
	}

	float cosAngle = std::cos(platform.currentTime);
	float sinAngle = std::sin(platform.currentTime);

	if (std::abs(platform.rotationAxis.y) > 0.9f)
	{
		transform.transition.x = platform.circleCenter.x + platform.circleRadius * cosAngle;
		transform.transition.y = platform.circleCenter.y;
		transform.transition.z = platform.circleCenter.z + platform.circleRadius * sinAngle;
	}
	else if (std::abs(platform.rotationAxis.x) > 0.9f)
	{
		transform.transition.x = platform.circleCenter.x;
		transform.transition.y = platform.circleCenter.y + platform.circleRadius * cosAngle;
		transform.transition.z = platform.circleCenter.z + platform.circleRadius * sinAngle;
	}
	else
	{
		transform.transition.x = platform.circleCenter.x + platform.circleRadius * cosAngle;
		transform.transition.y = platform.circleCenter.y + platform.circleRadius * sinAngle;
		transform.transition.z = platform.circleCenter.z;
	}
}

void PlatformSystem::UpdateSelfRotation(PlatformComponent& platform, TransformComponent& transform, float deltaTime)
{
	float angle = platform.selfRotationSpeed * deltaTime;
	Quaternion rotation = Quaternion::GetQuaternionFromAngleAxis(Radian(angle), platform.selfRotationAxis);
	transform.rotation = transform.rotation * rotation;
	transform.rotation.Normalize();
}

Entity PlatformSystem::CreatePlatform(World& world, const PlatformCreateInfo& info)
{
	auto platformObj = world.CreateGameObject(info.name);
	Entity entity = platformObj->GetHandle();

	// 添加TransformComponent，scale使用platformSize
	world.AddComponent<TransformComponent>(entity, info.position, Quaternion::IDENTITY, info.size);

	// 加载模型并添加MeshComponent和MaterialComponent
	if (!info.modelPath.empty())
	{
		auto model = AssetManager::LoadModel(info.modelPath.c_str());
		if (model)
		{
			world.AddComponent<MaterialComponent>(entity, model);
			world.AddComponent<MeshComponent>(entity, model);
		}
	}

	// 添加PlatformComponent
	PlatformComponent platformComp;
	platformComp.modelPath = info.modelPath;
	platformComp.startPosition = info.position;
	platformComp.endPosition = info.position + Vector3(10.0f, 0.0f, 0.0f);
	platformComp.platformSize = info.size;
	platformComp.motionType = info.motionType;
	platformComp.platformName = info.name;
	platformComp.platformId = mNextPlatformId++;
	world.AddComponent<PlatformComponent>(entity, std::move(platformComp));

	// 添加DynamicVoxelBodyComponent用于碰撞
	DynamicVoxelBodyComponent bodyComp;
	bodyComp.halfExtentsInVoxels = info.size * 0.5f / info.voxelSize;
	bodyComp.voxelSize = info.voxelSize;
	bodyComp.isPlatform = true;
	bodyComp.pushEntities = true;
	bodyComp.isOneWay = false;
	bodyComp.rotateRiders = true;
	bodyComp.rotation = Quaternion::IDENTITY;
	bodyComp.previousRotation = Quaternion::IDENTITY;
	world.AddComponent<DynamicVoxelBodyComponent>(entity, bodyComp);

	return entity;
}

void PlatformSystem::DestroyPlatform(World& world, Entity entity)
{
	world.DestroyGameObject(entity);
}

bool PlatformSystem::SaveToJson(World& world, const std::string& filePath)
{
	try
	{
		json root;
		json platformsArray = json::array();

		auto& registry = world.GetRegistry();
		auto view = registry.view<TransformComponent, PlatformComponent, DynamicVoxelBodyComponent>();

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& platform = view.get<PlatformComponent>(entity);
			auto& body = view.get<DynamicVoxelBodyComponent>(entity);

			json platformJson;

			platformJson["name"] = platform.platformName;
			platformJson["id"] = platform.platformId;

			platformJson["position"] = Vec3ToJson(transform.transition);
			platformJson["rotation"] = QuatToJson(transform.rotation);

			platformJson["size"] = Vec3ToJson(platform.platformSize);
			platformJson["modelPath"] = WStringToUTF8(platform.modelPath);

			platformJson["motionType"] = static_cast<int>(platform.motionType);
			platformJson["isMoving"] = platform.isMoving;

			platformJson["startPosition"] = Vec3ToJson(platform.startPosition);
			platformJson["endPosition"] = Vec3ToJson(platform.endPosition);
			platformJson["moveSpeed"] = platform.moveSpeed;
			platformJson["pingPong"] = platform.pingPong;

			platformJson["circleCenter"] = Vec3ToJson(platform.circleCenter);
			platformJson["circleRadius"] = platform.circleRadius;
			platformJson["rotationSpeed"] = platform.rotationSpeed;
			platformJson["rotationAxis"] = Vec3ToJson(platform.rotationAxis);

			platformJson["enableRotation"] = platform.enableRotation;
			platformJson["selfRotationAxis"] = Vec3ToJson(platform.selfRotationAxis);
			platformJson["selfRotationSpeed"] = platform.selfRotationSpeed;

			platformJson["voxelSize"] = body.voxelSize;
			platformJson["isPlatform"] = body.isPlatform;
			platformJson["pushEntities"] = body.pushEntities;
			platformJson["isOneWay"] = body.isOneWay;
			platformJson["oneWayDirection"] = Vec3ToJson(body.oneWayDirection);

			platformsArray.push_back(platformJson);
		}

		root["platforms"] = platformsArray;

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

bool PlatformSystem::LoadFromJson(World& world, const std::string& filePath)
{
	try
	{
		std::ifstream file(filePath);
		if (!file.is_open()) return false;

		json root;
		file >> root;
		file.close();

		if (!root.contains("platforms")) return false;

		auto& platformsArray = root["platforms"];

		for (auto& platformJson : platformsArray)
		{
			PlatformCreateInfo info;
			info.name = platformJson.value("name", "Platform");
			info.position = JsonToVec3(platformJson["position"]);
			info.size = JsonToVec3(platformJson["size"]);
			info.motionType = static_cast<PlatformMotionType>(platformJson.value("motionType", 0));
			info.voxelSize = platformJson.value("voxelSize", 2.0f);
			info.modelPath = UTF8ToWString(platformJson.value("modelPath", "Asset/Models/Cube/cube.obj"));

			Entity entity = CreatePlatform(world, info);

			auto& registry = world.GetRegistry();
			if (registry.valid(entity))
			{
				auto& transform = registry.get<TransformComponent>(entity);
				auto& platform = registry.get<PlatformComponent>(entity);
				auto& body = registry.get<DynamicVoxelBodyComponent>(entity);

				transform.rotation = JsonToQuat(platformJson["rotation"]);

				platform.isMoving = platformJson.value("isMoving", true);

				platform.startPosition = JsonToVec3(platformJson["startPosition"]);
				platform.endPosition = JsonToVec3(platformJson["endPosition"]);
				platform.moveSpeed = platformJson.value("moveSpeed", 5.0f);
				platform.pingPong = platformJson.value("pingPong", true);

				platform.circleCenter = JsonToVec3(platformJson["circleCenter"]);
				platform.circleRadius = platformJson.value("circleRadius", 5.0f);
				platform.rotationSpeed = platformJson.value("rotationSpeed", 1.0f);
				platform.rotationAxis = JsonToVec3(platformJson["rotationAxis"]);

				platform.enableRotation = platformJson.value("enableRotation", false);
				platform.selfRotationAxis = JsonToVec3(platformJson["selfRotationAxis"]);
				platform.selfRotationSpeed = platformJson.value("selfRotationSpeed", 0.0f);

				body.isPlatform = platformJson.value("isPlatform", true);
				body.pushEntities = platformJson.value("pushEntities", true);
				body.isOneWay = platformJson.value("isOneWay", false);
				body.oneWayDirection = JsonToVec3(platformJson["oneWayDirection"]);
			}
		}

		return true;
	}
	catch (...)
	{
		return false;
	}
}
