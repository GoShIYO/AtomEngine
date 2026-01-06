#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include "DynamicVoxelBodyComponent.h"
#include <string>

using namespace AtomEngine;

enum class PlatformMotionType
{
	Static = 0,
	Linear = 1,
	Circular = 2,
	Custom = 3
};

struct PlatformComponent
{
	std::wstring modelPath = L"";

	PlatformMotionType motionType{ PlatformMotionType::Linear };

	Vector3 startPosition{ 0.0f, 0.0f, 0.0f };
	Vector3 endPosition{ 10.0f, 0.0f, 0.0f };
	float moveSpeed{ 5.0f };
	bool pingPong{ true };

	Vector3 circleCenter{ 0.0f, 0.0f, 0.0f };
	float circleRadius{ 5.0f };
	float rotationSpeed{ 1.0f };
	Vector3 rotationAxis{ 0.0f, 1.0f, 0.0f };

	bool enableRotation{ false };
	Vector3 selfRotationAxis{ 0.0f, 1.0f, 0.0f };
	float selfRotationSpeed{ 0.0f };

	Vector3 platformSize{ 4.0f, 0.5f, 4.0f };

	float currentTime{ 0.0f };
	float direction{ 1.0f };
	bool isMoving{ true };

	bool isSelected{ false };
	bool showPath{ true };
	std::string platformName = "Platform";
	int platformId{ 0 };
};

struct PlatformCreateInfo
{
	Vector3 position{ 0.0f, 5.0f, 0.0f };
	Vector3 size{ 4.0f, 0.5f, 4.0f };
	PlatformMotionType motionType{ PlatformMotionType::Linear };
	float voxelSize{ 2.0f };
	std::wstring modelPath = L"Asset/Models/platform/platform.obj";
	std::string name = "Platform";
};