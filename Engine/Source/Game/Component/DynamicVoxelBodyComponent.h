#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include <vector>

struct DynamicVoxelBodyComponent
{
	struct VoxelCell
	{
		int x, y, z;
	};
	std::vector<VoxelCell> cells;

	AtomEngine::Vector3 halfExtentsInVoxels{ 1.0f, 1.0f, 1.0f };
	bool useSimpleBox{ true };

	float voxelSize{ 1.0f };

	bool isPlatform{ true };
	bool pushEntities{ true };
	bool isOneWay{ false };
	AtomEngine::Vector3 oneWayDirection{ 0.0f, 1.0f, 0.0f };

	bool rotateRiders{ true };

	AtomEngine::Vector3 velocity{ 0.0f, 0.0f, 0.0f };
	AtomEngine::Vector3 previousPosition{ 0.0f, 0.0f, 0.0f };

	AtomEngine::Quaternion rotation{ AtomEngine::Quaternion::IDENTITY };
	AtomEngine::Quaternion previousRotation{ AtomEngine::Quaternion::IDENTITY };
	AtomEngine::Vector3 angularVelocity{ 0.0f, 0.0f, 0.0f };

	uint32_t collisionLayer{ 1 };
	uint32_t collisionMask{ 0xFFFFFFFF };
};

struct DynamicVoxelCollisionEvent
{
	AtomEngine::Entity dynamicBody;
	AtomEngine::Entity otherEntity;
	AtomEngine::Vector3 contactPoint;
	AtomEngine::Vector3 normal;
	AtomEngine::Vector3 pushVector;
};

struct PlatformRideEvent
{
	AtomEngine::Entity platform;
	AtomEngine::Entity rider;
	bool isMount;
};
