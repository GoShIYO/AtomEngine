#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include <string>

struct LadderComponent
{
	AtomEngine::Vector3 halfExtents{ 0.5f, 2.0f, 0.5f };

	int ladderId{ 0 };

	std::string ladderName{ "Ladder" };

	float climbSpeed{ 5.0f };

	bool isEnabled{ true };

	bool isSelected{ false };

	float topExitForwardOffset{ 0.5f };

	float topExitUpOffset{ 0.1f };
};

struct LadderCreateInfo
{
	AtomEngine::Vector3 position{ 0.0f, 0.0f, 0.0f };
	AtomEngine::Vector3 halfExtents{ 0.5f, 2.0f, 0.5f };
	float climbSpeed{ 5.0f };
	std::string name{ "Ladder" };
};

struct LadderClimbEvent
{
	AtomEngine::Entity ladder;
	AtomEngine::Entity climber;
	bool isStarting;
};
