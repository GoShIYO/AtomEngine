#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include <string>

struct GoalComponent
{
	bool isReached{ false };
	float triggerRadius{ 3.0f };
};

struct GoalCreateInfo
{
	AtomEngine::Vector3 position{ 0.0f, 0.0f, 0.0f };
	std::wstring modelPath = L"";
	float scale{ 1.0f };
	float triggerRadius{ 3.0f };
	std::string name = "Goal";
};