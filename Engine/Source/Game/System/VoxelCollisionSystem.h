#pragma once
#include "Runtime/Function/Framework/ECS/World.h"
#include "../Voxel/VoxelWorld.h"
#include <unordered_set>
#include <unordered_map>

class VoxelCollisionSystem
{
public:
	VoxelCollisionSystem() = default;

	void SetVoxelWorld(VoxelWorld* world) { mVoxelWorld = world; }

	void SetGravity(float gravity) { mGravity = gravity; }
	float GetGravity() const { return mGravity; }

	void Update(AtomEngine::World& world, float deltaTime);

	void ProcessEntity(AtomEngine::World& world, AtomEngine::Entity entity);

	std::optional<VoxelHit> Raycast(const AtomEngine::Vector3& origin,
		const AtomEngine::Vector3& direction,
		float maxDistance = 1000.0f) const;

	std::optional<VoxelHit> RaycastStatic(const AtomEngine::Vector3& origin,
		const AtomEngine::Vector3& direction,
		float maxDistance = 1000.0f) const;

	bool IsPositionValid(const AtomEngine::Vector3& position,
		const AtomEngine::Vector3& halfExtents) const;

	AtomEngine::Entity GetPlatformUnder(AtomEngine::World& world, AtomEngine::Entity entity) const;

private:
	VoxelWorld* mVoxelWorld{ nullptr };
	float mGravity{ 40.0f };

	std::unordered_map<AtomEngine::Entity, std::unordered_set<AtomEngine::Entity>> mPlatformRiders;

	std::unordered_map<AtomEngine::Entity, int> mStuckFrameCount;

	std::unordered_map<AtomEngine::Entity, AtomEngine::Vector3> mLastPositions;

	static constexpr float kMinMovementThreshold = 0.001f;
	static constexpr int kMaxStuckFrames = 10;
	static constexpr float kEmergencyPushDistance = 0.5f;
	static constexpr float kSkinWidth = 0.01f;
	static constexpr float kMaxPenetrationResolve = 2.0f;

	void ApplyGravity(AtomEngine::World& world, AtomEngine::Entity entity, float deltaTime);

	void ProcessMovement(AtomEngine::World& world, AtomEngine::Entity entity, float deltaTime);

	void UpdateGroundedState(AtomEngine::World& world, AtomEngine::Entity entity);

	void ProcessStepClimbSmooth(AtomEngine::World& world, AtomEngine::Entity entity, float deltaTime);

	AtomEngine::Vector3 ClampToWorldBounds(const AtomEngine::Vector3& position,
		const AtomEngine::Vector3& halfExtents) const;

	bool TryStepClimb(const AtomEngine::Vector3& currentPos,
		const AtomEngine::Vector3& horizontalMovement,
		const AtomEngine::Vector3& halfExtents,
		float maxStepHeight,
		float stepSearchOvershoot,
		float minStepDepth,
		AtomEngine::Vector3& outNewPos) const;

	bool TryStepClimbOnPlatform(AtomEngine::World& world,
		AtomEngine::Entity entity,
		const AtomEngine::Vector3& currentPos,
		const AtomEngine::Vector3& horizontalMovement,
		const AtomEngine::Vector3& halfExtents,
		float maxStepHeight,
		AtomEngine::Vector3& outNewPos) const;

	void UpdateDynamicBodies(AtomEngine::World& world, float deltaTime);

	void ProcessDynamicBodyCollisions(AtomEngine::World& world, float deltaTime);

	void UpdatePlatformRiders(AtomEngine::World& world, float deltaTime);

	bool CheckDynamicBodyCollision(AtomEngine::World& world,
		const AtomEngine::Vector3& boxMin,
		const AtomEngine::Vector3& boxMax,
		AtomEngine::Entity excludeEntity,
		AtomEngine::Vector3& outPushVector,
		AtomEngine::Entity& outCollidedBody) const;

	void GetDynamicBodyAABB(AtomEngine::World& world, AtomEngine::Entity entity,
		AtomEngine::Vector3& outMin, AtomEngine::Vector3& outMax) const;

	bool ShouldCollideWithOneWay(const AtomEngine::Vector3& velocity,
		const AtomEngine::Vector3& oneWayDirection) const;

	bool IsEntityStuck(AtomEngine::World& world, AtomEngine::Entity entity,
		const AtomEngine::Vector3& currentPos);

	bool TryEmergencyEscape(AtomEngine::World& world, AtomEngine::Entity entity);

	AtomEngine::Vector3 FindNearestSafePosition(const AtomEngine::Vector3& position,
		const AtomEngine::Vector3& halfExtents) const;

	AtomEngine::Vector3 ResolveCollisionProgressive(
		const AtomEngine::Vector3& position,
		const AtomEngine::Vector3& halfExtents,
		float maxCorrectionPerFrame);

	bool SeparatingAxisTest(const AtomEngine::Vector3& boxMin1, const AtomEngine::Vector3& boxMax1,
		const AtomEngine::Vector3& boxMin2, const AtomEngine::Vector3& boxMax2,
		AtomEngine::Vector3& outMTV) const;
};
