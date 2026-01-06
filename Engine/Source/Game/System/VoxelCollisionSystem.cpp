#include "VoxelCollisionSystem.h"
#include "../Component/VoxelColliderComponent.h"
#include "../Component/DynamicVoxelBodyComponent.h"
#include "../Component/VelocityComponent.h"
#include "LadderSystem.h"  // 用于ClimbingStateComponent
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>

using namespace AtomEngine;

void VoxelCollisionSystem::Update(World& world, float deltaTime)
{
	if (!mVoxelWorld) return;

	UpdateDynamicBodies(world, deltaTime);

	auto& registry = world.GetRegistry();
	auto colliderView = registry.view<TransformComponent, VoxelColliderComponent>();
	for (auto entity : colliderView)
	{
		auto& collider = colliderView.get<VoxelColliderComponent>(entity);

		if (registry.any_of<ClimbingStateComponent>(entity))
		{
			continue;
		}

		UpdateGroundedState(world, entity);
		ApplyGravity(world, entity, deltaTime);

		ProcessStepClimbSmooth(world, entity, deltaTime);

		ProcessMovement(world, entity, deltaTime);

		auto& transform = colliderView.get<TransformComponent>(entity);
		if (IsEntityStuck(world, entity, transform.transition))
		{
			TryEmergencyEscape(world, entity);
		}
	}

	ProcessDynamicBodyCollisions(world, deltaTime);

	UpdatePlatformRiders(world, deltaTime);
}

bool VoxelCollisionSystem::IsEntityStuck(World& world, Entity entity, const Vector3& currentPos)
{
	auto it = mLastPositions.find(entity);
	if (it == mLastPositions.end())
	{
		mLastPositions[entity] = currentPos;
		mStuckFrameCount[entity] = 0;
		return false;
	}

	Vector3 lastPos = it->second;
	float movementSqr = (currentPos - lastPos).LengthSqr();

	auto& collider = world.GetComponent<VoxelColliderComponent>(entity);
	bool hasVelocity = false;

	if (world.HasComponent<VelocityComponent>(entity))
	{
		auto& vel = world.GetComponent<VelocityComponent>(entity);
		hasVelocity = vel.velocity.LengthSqr() > 0.1f;
	}
	hasVelocity |= std::abs(collider.verticalVelocity) > 0.1f;

	if (hasVelocity&& movementSqr < kMinMovementThreshold* kMinMovementThreshold)
	{
		mStuckFrameCount[entity]++;
	}
	else
	{
		mStuckFrameCount[entity] = 0;
	}

	mLastPositions[entity] = currentPos;

	return mStuckFrameCount[entity] > kMaxStuckFrames;
}

bool VoxelCollisionSystem::TryEmergencyEscape(World& world, Entity entity)
{
	auto& transform = world.GetComponent<TransformComponent>(entity);
	auto& collider = world.GetComponent<VoxelColliderComponent>(entity);

	Vector3 position = transform.transition + collider.offset;
	Vector3 halfExtents = collider.halfExtents;

	// 尝试向各个方向推出
	const Vector3 escapeDirections[] = {
		Vector3(0, 1, 0),   // 上
		Vector3(0, -1, 0),  // 下
		Vector3(1, 0, 0),   // 右
		Vector3(-1, 0, 0),  // 左
		Vector3(0, 0, 1),
		Vector3(0, 0, -1),
		Vector3(0, 2, 0),
	};

	for (const auto& dir : escapeDirections)
	{
		for (float distance = kEmergencyPushDistance; distance <= 3.0f; distance += 0.5f)
		{
			Vector3 testPos = position + dir * distance;

			Vector3 testMin = testPos - halfExtents;
			Vector3 testMax = testPos + halfExtents;

			if (!mVoxelWorld->OverlapsSolid(testMin, testMax))
			{
				Vector3 pushVector;
				Entity collidedBody;
				if (!CheckDynamicBodyCollision(world, testMin, testMax, entity, pushVector, collidedBody))
				{
					transform.transition = testPos - collider.offset;
					collider.verticalVelocity = 0.0f;
					mStuckFrameCount[entity] = 0;

					if (world.HasComponent<VelocityComponent>(entity))
					{
						world.GetComponent<VelocityComponent>(entity).velocity = Vector3::ZERO;
					}

					return true;
				}
			}
		}
	}

	Vector3 safePos = FindNearestSafePosition(position, halfExtents);
	if ((safePos - position).LengthSqr() > 0.01f)
	{
		transform.transition = safePos - collider.offset;
		collider.verticalVelocity = 0.0f;
		mStuckFrameCount[entity] = 0;
		return true;
	}

	return false;
}

Vector3 VoxelCollisionSystem::FindNearestSafePosition(const Vector3& position,
	const Vector3& halfExtents) const
{
	const float searchRadius = 5.0f;
	const float step = mVoxelWorld->voxelSize;

	float bestDistSqr = FLT_MAX;
	Vector3 bestPos = position;

	for (float y = -searchRadius; y <= searchRadius; y += step)
	{
		for (float x = -searchRadius; x <= searchRadius; x += step)
		{
			for (float z = -searchRadius; z <= searchRadius; z += step)
			{
				Vector3 testPos = position + Vector3(x, y, z);
				float distSqr = (testPos - position).LengthSqr();

				if (distSqr >= bestDistSqr) continue;

				Vector3 testMin = testPos - halfExtents;
				Vector3 testMax = testPos + halfExtents;

				if (!mVoxelWorld->OverlapsSolid(testMin, testMax))
				{
					float adjustedDist = distSqr - y * 0.5f;
					if (adjustedDist < bestDistSqr)
					{
						bestDistSqr = adjustedDist;
						bestPos = testPos;
					}
				}
			}
		}
	}

	return bestPos;
}

bool VoxelCollisionSystem::SeparatingAxisTest(
	const Vector3& boxMin1, const Vector3& boxMax1,
	const Vector3& boxMin2, const Vector3& boxMax2,
	Vector3& outMTV) const
{
	if (boxMax1.x < boxMin2.x || boxMin1.x > boxMax2.x) return false;
	if (boxMax1.y < boxMin2.y || boxMin1.y > boxMax2.y) return false;
	if (boxMax1.z < boxMin2.z || boxMin1.z > boxMax2.z) return false;

	float overlapX1 = boxMax2.x - boxMin1.x;
	float overlapX2 = boxMax1.x - boxMin2.x;
	float overlapY1 = boxMax2.y - boxMin1.y;
	float overlapY2 = boxMax1.y - boxMin2.y;
	float overlapZ1 = boxMax2.z - boxMin1.z;
	float overlapZ2 = boxMax1.z - boxMin2.z;

	float minOverlap = FLT_MAX;
	outMTV = Vector3::ZERO;

	if (overlapX1 > 0 && overlapX1 < minOverlap)
	{
		minOverlap = overlapX1;
		outMTV = Vector3(overlapX1, 0, 0);
	}
	if (overlapX2 > 0 && overlapX2 < minOverlap)
	{
		minOverlap = overlapX2;
		outMTV = Vector3(-overlapX2, 0, 0);
	}
	if (overlapY1 > 0 && overlapY1 < minOverlap)
	{
		minOverlap = overlapY1;
		outMTV = Vector3(0, overlapY1, 0);
	}
	if (overlapY2 > 0 && overlapY2 < minOverlap)
	{
		minOverlap = overlapY2;
		outMTV = Vector3(0, -overlapY2, 0);
	}
	if (overlapZ1 > 0 && overlapZ1 < minOverlap)
	{
		minOverlap = overlapZ1;
		outMTV = Vector3(0, 0, overlapZ1);
	}
	if (overlapZ2 > 0 && overlapZ2 < minOverlap)
	{
		minOverlap = overlapZ2;
		outMTV = Vector3(0, 0, -overlapZ2);
	}

	return true;
}

Vector3 VoxelCollisionSystem::ResolveCollisionProgressive(
	const Vector3& position,
	const Vector3& halfExtents,
	float maxCorrectionPerFrame)
{
	Vector3 boxMin = position - halfExtents;
	Vector3 boxMax = position + halfExtents;

	VoxelCollisionResult result = mVoxelWorld->ResolveAABBCollision(boxMin, boxMax);

	if (!result.collided) return position;

	Vector3 correction = result.correctedPosition - position;
	float correctionLength = correction.Length();

	if (correctionLength > maxCorrectionPerFrame)
	{
		correction = correction.NormalizedCopy() * maxCorrectionPerFrame;
	}

	return position + correction;
}

void VoxelCollisionSystem::UpdateDynamicBodies(World& world, float deltaTime)
{
	auto& registry = world.GetRegistry();
	auto view = registry.view<TransformComponent, DynamicVoxelBodyComponent>();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& body = view.get<DynamicVoxelBodyComponent>(entity);

		Vector3 currentPos = transform.transition;
		if (deltaTime > 0.0001f)
		{
			Vector3 newVelocity = (currentPos - body.previousPosition) / deltaTime;
			body.velocity = body.velocity * 0.3f + newVelocity * 0.7f;
		}

		body.previousPosition = currentPos;
	}
}

void VoxelCollisionSystem::GetDynamicBodyAABB(World& world, Entity entity,
	Vector3& outMin, Vector3& outMax) const
{
	auto& transform = world.GetComponent<TransformComponent>(entity);
	auto& body = world.GetComponent<DynamicVoxelBodyComponent>(entity);

	Vector3 center = transform.transition;

	if (body.useSimpleBox)
	{
		Vector3 halfExtents = body.halfExtentsInVoxels * body.voxelSize;
		outMin = center - halfExtents;
		outMax = center + halfExtents;
	}
	else
	{
		if (body.cells.empty())
		{
			outMin = center;
			outMax = center;
			return;
		}

		int minX = INT_MAX, minY = INT_MAX, minZ = INT_MAX;
		int maxX = INT_MIN, maxY = INT_MIN, maxZ = INT_MIN;

		for (const auto& cell : body.cells)
		{
			minX = std::min(minX, cell.x);
			minY = std::min(minY, cell.y);
			minZ = std::min(minZ, cell.z);
			maxX = std::max(maxX, cell.x);
			maxY = std::max(maxY, cell.y);
			maxZ = std::max(maxZ, cell.z);
		}

		outMin = center + Vector3((float)minX, (float)minY, (float)minZ) * body.voxelSize;
		outMax = center + Vector3((float)(maxX + 1), (float)(maxY + 1), (float)(maxZ + 1)) * body.voxelSize;
	}
}

bool VoxelCollisionSystem::ShouldCollideWithOneWay(const Vector3& velocity,
	const Vector3& oneWayDirection) const
{
	return velocity.Dot(oneWayDirection) <= 0.0f;
}

bool VoxelCollisionSystem::CheckDynamicBodyCollision(World& world,
	const Vector3& boxMin,
	const Vector3& boxMax,
	Entity excludeEntity,
	Vector3& outPushVector,
	Entity& outCollidedBody) const
{
	auto& registry = world.GetRegistry();
	auto view = registry.view<TransformComponent, DynamicVoxelBodyComponent>();

	float minPenetration = FLT_MAX;
	bool foundCollision = false;
	Entity closestBody = entt::null;
	Vector3 bestPushVector = Vector3::ZERO;

	for (auto entity : view)
	{
		if (entity == excludeEntity) continue;

		auto& body = view.get<DynamicVoxelBodyComponent>(entity);

		Vector3 bodyMin, bodyMax;
		const_cast<VoxelCollisionSystem*>(this)->GetDynamicBodyAABB(
			const_cast<World&>(world), entity, bodyMin, bodyMax);

		Vector3 mtv;
		if (SeparatingAxisTest(boxMin, boxMax, bodyMin, bodyMax, mtv))
		{
			float penDepth = mtv.Length();
			if (penDepth < minPenetration)
			{
				minPenetration = penDepth;
				bestPushVector = mtv;
				closestBody = entity;
				foundCollision = true;
			}
		}
	}

	if (foundCollision)
	{
		outPushVector = bestPushVector;
		outCollidedBody = closestBody;
	}

	return foundCollision;
}

void VoxelCollisionSystem::ProcessDynamicBodyCollisions(World& world, float deltaTime)
{
	auto& registry = world.GetRegistry();
	auto colliderView = registry.view<TransformComponent, VoxelColliderComponent>();

	for (auto entity : colliderView)
	{
		auto& transform = colliderView.get<TransformComponent>(entity);
		auto& collider = colliderView.get<VoxelColliderComponent>(entity);

		if (!collider.enableCollision) continue;

		if (registry.any_of<ClimbingStateComponent>(entity))
		{
			continue;
		}

		Vector3 position = transform.transition + collider.offset;

		Vector3 expandedHalfExtents = collider.halfExtents + Vector3(kSkinWidth, kSkinWidth, kSkinWidth);
		Vector3 boxMin = position - expandedHalfExtents;
		Vector3 boxMax = position + expandedHalfExtents;

		Vector3 pushVector;
		Entity collidedBody;

		if (CheckDynamicBodyCollision(world, boxMin, boxMax, entity, pushVector, collidedBody))
		{
			auto& body = world.GetComponent<DynamicVoxelBodyComponent>(collidedBody);

			if (body.isOneWay)
			{
				Vector3 entityVel = Vector3::ZERO;
				if (world.HasComponent<VelocityComponent>(entity))
				{
					entityVel = world.GetComponent<VelocityComponent>(entity).velocity;
				}
				entityVel.y = collider.verticalVelocity;

				if (!ShouldCollideWithOneWay(entityVel, body.oneWayDirection))
				{
					continue;
				}
			}

			if (body.pushEntities)
			{
				float pushLength = pushVector.Length();
				if (pushLength > kMaxPenetrationResolve)
				{
					pushVector = pushVector.NormalizedCopy() * kMaxPenetrationResolve;
				}

				if (pushLength > 0.001f)
				{
					pushVector += pushVector.NormalizedCopy() * kSkinWidth;
				}

				transform.transition += pushVector;

				if (std::abs(pushVector.y) > 0.001f)
				{
					if (pushVector.y > 0)
					{
						collider.verticalVelocity = std::max(0.0f, collider.verticalVelocity);
						collider.isGrounded = true;
					}
					else
					{
						collider.verticalVelocity = std::min(0.0f, collider.verticalVelocity);
					}
				}

				world.GetDispatcher().trigger<DynamicVoxelCollisionEvent>({
					collidedBody,
					entity,
					position,
					pushVector.LengthSqr() > 0.0001f ? pushVector.NormalizedCopy() : Vector3::UP,
					pushVector
					});
			}
		}
	}
}

void VoxelCollisionSystem::UpdatePlatformRiders(World& world, float deltaTime)
{
	auto& registry = world.GetRegistry();
	auto bodyView = registry.view<TransformComponent, DynamicVoxelBodyComponent>();

	for (auto it = mPlatformRiders.begin(); it != mPlatformRiders.end();)
	{
		if (!registry.valid(it->first))
		{
			it = mPlatformRiders.erase(it);
		}
		else
		{
			auto& riders = it->second;
			for (auto riderIt = riders.begin(); riderIt != riders.end();)
			{
				if (!registry.valid(*riderIt))
				{
					riderIt = riders.erase(riderIt);
				}
				else
				{
					++riderIt;
				}
			}
			++it;
		}
	}

	auto colliderView = registry.view<TransformComponent, VoxelColliderComponent>();

	for (auto entity : colliderView)
	{
		auto& transform = colliderView.get<TransformComponent>(entity);
		auto& collider = colliderView.get<VoxelColliderComponent>(entity);

		Vector3 position = transform.transition + collider.offset;

		Vector3 footMin = position - collider.halfExtents;
		footMin.y -= collider.groundCheckDistance + 0.1f;
		Vector3 footMax = position + collider.halfExtents;
		footMax.y = position.y - collider.halfExtents.y + 0.05f;

		for (auto bodyEntity : bodyView)
		{
			auto& platformTransform = bodyView.get<TransformComponent>(bodyEntity);
			auto& body = bodyView.get<DynamicVoxelBodyComponent>(bodyEntity);

			if (!body.isPlatform) continue;

			Vector3 bodyMin, bodyMax;
			GetDynamicBodyAABB(world, bodyEntity, bodyMin, bodyMax);

			bool overlapsXZ = (footMax.x > bodyMin.x + 0.01f && footMin.x < bodyMax.x - 0.01f &&
				footMax.z > bodyMin.z + 0.01f && footMin.z < bodyMax.z - 0.01f);

			float feetY = position.y - collider.halfExtents.y;
			float platformTopY = bodyMax.y;
			bool onTop = (feetY >= platformTopY - 0.2f && feetY <= platformTopY + 0.3f);

			if (overlapsXZ && onTop)
			{
				auto& riders = mPlatformRiders[bodyEntity];
				bool wasRiding = riders.count(entity) > 0;

				if (!wasRiding)
				{
					riders.insert(entity);
					world.GetDispatcher().trigger<PlatformRideEvent>({
		   bodyEntity, entity, true
						});
				}

				if (body.velocity.LengthSqr() > 0.0001f)
				{
					transform.transition += body.velocity * deltaTime;

					float targetY = platformTopY + collider.halfExtents.y + 0.01f;
					if (transform.transition.y < targetY - collider.offset.y)
					{
						transform.transition.y = targetY - collider.offset.y;
					}
				}

				if (body.rotateRiders && body.angularVelocity.LengthSqr() > 0.0001f)
				{
					Vector3 platformCenter = platformTransform.transition;

					Vector3 riderPos = transform.transition;
					Vector3 relativePos = riderPos - platformCenter;

					Quaternion rotDelta = body.rotation * body.previousRotation.Inverse();
					rotDelta.Normalize();

					Vector3 rotatedRelativePos = rotDelta * relativePos;

					transform.transition = platformCenter + rotatedRelativePos;

					transform.rotation = rotDelta * transform.rotation;
					transform.rotation.Normalize();
				}

				collider.isGrounded = true;
			}
			else
			{
				auto& riders = mPlatformRiders[bodyEntity];
				if (riders.count(entity) > 0)
				{
					riders.erase(entity);
					world.GetDispatcher().trigger<PlatformRideEvent>({
					  bodyEntity, entity, false
						});
				}
			}
		}
	}
}

Entity VoxelCollisionSystem::GetPlatformUnder(World& world, Entity entity) const
{
	for (const auto& [platform, riders] : mPlatformRiders)
	{
		if (riders.count(entity) > 0)
		{
			return platform;
		}
	}
	return entt::null;
}

std::optional<VoxelHit> VoxelCollisionSystem::RaycastStatic(const Vector3& origin,
	const Vector3& direction,
	float maxDistance) const
{
	if (!mVoxelWorld) return std::nullopt;
	return mVoxelWorld->Raycast(origin, direction, maxDistance);
}

std::optional<VoxelHit> VoxelCollisionSystem::Raycast(const Vector3& origin,
	const Vector3& direction,
	float maxDistance) const
{
	if (!mVoxelWorld) return std::nullopt;
	return mVoxelWorld->Raycast(origin, direction, maxDistance);
}

Vector3 VoxelCollisionSystem::ClampToWorldBounds(const Vector3& position,
	const Vector3& halfExtents) const
{
	if (!mVoxelWorld) return position;

	Vector3 result = position;

	Vector3 worldMin = mVoxelWorld->origin;
	Vector3 worldMax = mVoxelWorld->origin + Vector3(
		mVoxelWorld->width * mVoxelWorld->voxelSize,
		mVoxelWorld->height * mVoxelWorld->voxelSize,
		mVoxelWorld->depth * mVoxelWorld->voxelSize
	);

	// X轴
	if (result.x - halfExtents.x < worldMin.x)
	{
		result.x = worldMin.x + halfExtents.x;
	}
	else if (result.x + halfExtents.x > worldMax.x)
	{
		result.x = worldMax.x - halfExtents.x;
	}

	// Y轴
	if (result.y - halfExtents.y < worldMin.y)
	{
		result.y = worldMin.y + halfExtents.y;
	}
	// else if (result.y + halfExtents.y > worldMax.y)
	// {
	//   result.y = worldMax.y - halfExtents.y;
	// }

	// Z轴
	if (result.z - halfExtents.z < worldMin.z)
	{
		result.z = worldMin.z + halfExtents.z;
	}
	else if (result.z + halfExtents.z > worldMax.z)
	{
		result.z = worldMax.z - halfExtents.z;
	}

	return result;
}

void VoxelCollisionSystem::ApplyGravity(World& world, Entity entity, float deltaTime)
{
	auto& collider = world.GetComponent<VoxelColliderComponent>(entity);

	if (!collider.useGravity) return;

	if (collider.isGrounded)
	{
		if (collider.verticalVelocity < 0.0f)
		{
			collider.verticalVelocity = 0.0f;
		}
	}
	else
	{
		collider.verticalVelocity -= mGravity * collider.gravityScale * deltaTime;
		collider.verticalVelocity = std::max(collider.verticalVelocity, -collider.maxFallSpeed);
	}
}

void VoxelCollisionSystem::ProcessEntity(World& world, Entity entity)
{
	if (!mVoxelWorld) return;

	if (!world.HasComponent<TransformComponent>(entity) ||
		!world.HasComponent<VoxelColliderComponent>(entity))
	{
		return;
	}

	auto& transform = world.GetComponent<TransformComponent>(entity);
	auto& collider = world.GetComponent<VoxelColliderComponent>(entity);

	if (!collider.enableCollision) return;

	Vector3 position = transform.transition + collider.offset;
	Vector3 boxMin = position - collider.halfExtents;
	Vector3 boxMax = position + collider.halfExtents;

	VoxelCollisionResult result = mVoxelWorld->ResolveAABBCollision(boxMin, boxMax);

	if (result.collided)
	{
		transform.transition = result.correctedPosition - collider.offset;
		collider.wasColliding = true;
		collider.lastCollisionNormal = result.normal;

		auto overlapping = mVoxelWorld->GetOverlappingVoxels(boxMin, boxMax);
		if (!overlapping.empty())
		{
			auto& coord = overlapping[0];
			world.GetDispatcher().trigger<VoxelCollisionEvent>({
		entity,
 result.correctedPosition,
		  result.normal,
		   coord.x, coord.y, coord.z
				});
		}
	}
	else
	{
		collider.wasColliding = false;
	}
}

void VoxelCollisionSystem::ProcessMovement(World& world, Entity entity, float deltaTime)
{
	auto& transform = world.GetComponent<TransformComponent>(entity);
	auto& collider = world.GetComponent<VoxelColliderComponent>(entity);

	if (!collider.enableCollision) return;

	if (collider.isClimbing && collider.smoothStepClimbing)
	{
		return;
	}

	Vector3 currentPos = transform.transition + collider.offset;
	Vector3 movement = Vector3::ZERO;

	if (world.HasComponent<VelocityComponent>(entity))
	{
		auto& velocity = world.GetComponent<VelocityComponent>(entity);
		movement = velocity.velocity * deltaTime;
	}

	if (collider.useGravity)
	{
		movement.y += collider.verticalVelocity * deltaTime;
	}

	if (movement.LengthSqr() < 0.00001f)
	{
		return;
	}

	Vector3 horizontalMovement = movement;
	horizontalMovement.y = 0;

	float effectiveMaxStepHeight = collider.maxStepHeight;
	if (collider.useAutoStepHeight)
	{
		effectiveMaxStepHeight = collider.halfExtents.y;
	}

	float maxMovePerFrame = mVoxelWorld->voxelSize * 0.9f;
	float movementLength = movement.Length();
	int subSteps = 1;

	if (movementLength > maxMovePerFrame)
	{
		subSteps = (int)std::ceil(movementLength / maxMovePerFrame);
		subSteps = std::min(subSteps, 8);
	}

	Vector3 stepMovement = movement / (float)subSteps;
	Vector3 stepHorizontalMovement = horizontalMovement / (float)subSteps;

	for (int step = 0; step < subSteps; ++step)
	{
		bool blockedHorizontally = false;

		if (collider.enableSliding)
		{
			Vector3 newPos = mVoxelWorld->MoveAndSlide(
				currentPos,
				collider.halfExtents,
				stepMovement,
				collider.maxSlideIterations
			);

			Vector3 actualMovement = newPos - currentPos;
			Vector3 blockedMovement = stepMovement - actualMovement;

			if (blockedMovement.LengthSqr() > 0.0001f)
			{
				collider.wasColliding = true;

				Vector3 blockedDir = blockedMovement.NormalizedCopy();
				collider.lastCollisionNormal = blockedDir;

				Vector3 horizontalBlocked = blockedMovement;
				horizontalBlocked.y = 0;
				blockedHorizontally = horizontalBlocked.LengthSqr() > 0.0001f;

				if (std::abs(blockedDir.y) > 0.5f)
				{
					if (blockedDir.y > 0 && collider.verticalVelocity < 0)
					{
						collider.verticalVelocity = 0.0f;
					}
					else if (blockedDir.y < 0 && collider.verticalVelocity > 0)
					{
						collider.verticalVelocity = 0.0f;
					}
				}

				if (world.HasComponent<VelocityComponent>(entity))
				{
					auto& velocity = world.GetComponent<VelocityComponent>(entity);

					Vector3 horizontalBlockedDir = blockedDir;
					horizontalBlockedDir.y = 0;
					if (horizontalBlockedDir.LengthSqr() > 0.0001f)
					{
						horizontalBlockedDir.Normalize();
						float normalComponent = velocity.velocity.Dot(horizontalBlockedDir);
						if (normalComponent > 0)
						{
							velocity.velocity = velocity.velocity - horizontalBlockedDir * normalComponent;
						}
					}
				}
			}
			else
			{
				collider.wasColliding = false;
			}

			currentPos = newPos;

			bool shouldCheckStepClimb = collider.enableStepClimbing &&
				collider.isGrounded &&
				stepHorizontalMovement.LengthSqr() > 0.0001f;

			if (shouldCheckStepClimb && (blockedHorizontally || true))
			{
				Vector3 stepClimbPos;
				Vector3 platformClimbPos;
				bool voxelClimbSuccess = false;
				bool platformClimbSuccess = false;

				voxelClimbSuccess = TryStepClimb(currentPos, stepHorizontalMovement,
					collider.halfExtents, effectiveMaxStepHeight,
					collider.stepSearchOvershoot, collider.minStepDepth, stepClimbPos);

				platformClimbSuccess = TryStepClimbOnPlatform(world, entity, currentPos,
					stepHorizontalMovement, collider.halfExtents,
					effectiveMaxStepHeight, platformClimbPos);

				Vector3 finalClimbPos;
				bool climbSuccess = false;

				if (voxelClimbSuccess && platformClimbSuccess)
				{
					float voxelHeightDiff = std::abs(stepClimbPos.y - currentPos.y);
					float platformHeightDiff = std::abs(platformClimbPos.y - currentPos.y);

					if (voxelHeightDiff <= platformHeightDiff)
					{
						finalClimbPos = stepClimbPos;
					}
					else
					{
						finalClimbPos = platformClimbPos;
					}
					climbSuccess = true;
				}
				else if (voxelClimbSuccess)
				{
					finalClimbPos = stepClimbPos;
					climbSuccess = true;
				}
				else if (platformClimbSuccess)
				{
					finalClimbPos = platformClimbPos;
					climbSuccess = true;
				}

				if (climbSuccess)
				{
					if (collider.smoothStepClimbing)
					{
						collider.isClimbing = true;
						collider.climbTargetPos = finalClimbPos - collider.offset;
						collider.verticalVelocity = 0.0f;
						collider.isGrounded = true;
						transform.transition = currentPos - collider.offset;
						return;
					}
					else
					{
						currentPos = finalClimbPos;
						collider.verticalVelocity = 0.0f;
						collider.isGrounded = true;
					}
				}
			}
		}
		else
		{
			Vector3 boxMin = currentPos - collider.halfExtents;
			Vector3 boxMax = currentPos + collider.halfExtents;

			VoxelSweepResult sweep = mVoxelWorld->SweepAABB(boxMin, boxMax, stepMovement);

			if (sweep.hit)
			{
				Vector3 newPos = currentPos + stepMovement * sweep.time;

				Vector3 horizontalNormal = sweep.normal;
				horizontalNormal.y = 0;
				blockedHorizontally = horizontalNormal.LengthSqr() > 0.5f;

				currentPos = newPos;

				if (std::abs(sweep.normal.y) > 0.5f)
				{
					collider.verticalVelocity = 0.0f;
				}

				if (world.HasComponent<VelocityComponent>(entity))
				{
					auto& velocity = world.GetComponent<VelocityComponent>(entity);
					if (horizontalNormal.LengthSqr() > 0.0001f)
					{
						horizontalNormal.Normalize();
						float normalComponent = velocity.velocity.Dot(horizontalNormal);
						if (normalComponent < 0)
						{
							velocity.velocity = velocity.velocity - horizontalNormal * normalComponent;
						}
					}
				}

				collider.wasColliding = true;
				collider.lastCollisionNormal = sweep.normal;

				bool shouldCheckStepClimb = collider.enableStepClimbing &&
					collider.isGrounded &&
					stepHorizontalMovement.LengthSqr() > 0.0001f;

				if (shouldCheckStepClimb && (blockedHorizontally || true))
				{
					Vector3 stepClimbPos;
					Vector3 platformClimbPos;
					bool voxelClimbSuccess = false;
					bool platformClimbSuccess = false;

					voxelClimbSuccess = TryStepClimb(currentPos, stepHorizontalMovement,
						collider.halfExtents, effectiveMaxStepHeight,
						collider.stepSearchOvershoot, collider.minStepDepth, stepClimbPos);

					platformClimbSuccess = TryStepClimbOnPlatform(world, entity, currentPos,
						stepHorizontalMovement, collider.halfExtents,
						effectiveMaxStepHeight, platformClimbPos);

					Vector3 finalClimbPos;
					bool climbSuccess = false;

					if (voxelClimbSuccess && platformClimbSuccess)
					{
						float voxelHeightDiff = std::abs(stepClimbPos.y - currentPos.y);
						float platformHeightDiff = std::abs(platformClimbPos.y - currentPos.y);

						if (voxelHeightDiff <= platformHeightDiff)
						{
							finalClimbPos = stepClimbPos;
						}
						else
						{
							finalClimbPos = platformClimbPos;
						}
						climbSuccess = true;
					}
					else if (voxelClimbSuccess)
					{
						finalClimbPos = stepClimbPos;
						climbSuccess = true;
					}
					else if (platformClimbSuccess)
					{
						finalClimbPos = platformClimbPos;
						climbSuccess = true;
					}

					if (climbSuccess)
					{
						if (collider.smoothStepClimbing)
						{
							collider.isClimbing = true;
							collider.climbTargetPos = finalClimbPos - collider.offset;
							collider.verticalVelocity = 0.0f;
							collider.isGrounded = true;
							transform.transition = currentPos - collider.offset;
							return;
						}
						else
						{
							currentPos = finalClimbPos;
							collider.verticalVelocity = 0.0f;
							collider.isGrounded = true;
						}
					}
					else
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			else
			{
				currentPos += stepMovement;
				collider.wasColliding = false;
			}
		}
	}

	currentPos = ClampToWorldBounds(currentPos, collider.halfExtents);

	transform.transition = currentPos - collider.offset;
}

void VoxelCollisionSystem::UpdateGroundedState(World& world, Entity entity)
{
	auto& transform = world.GetComponent<TransformComponent>(entity);
	auto& collider = world.GetComponent<VoxelColliderComponent>(entity);

	Vector3 position = transform.transition + collider.offset;

	bool wasGrounded = collider.isGrounded;

	bool groundedOnStatic = mVoxelWorld->IsGrounded(
		position,
		collider.halfExtents,
		collider.groundCheckDistance
	);

	bool groundedOnDynamic = (GetPlatformUnder(const_cast<World&>(world), entity) != entt::null);

	collider.isGrounded = groundedOnStatic || groundedOnDynamic;

	if (wasGrounded != collider.isGrounded)
	{
		world.GetDispatcher().trigger<GroundedStateChangedEvent>({
			entity,
		  collider.isGrounded
			});
	}
}

void VoxelCollisionSystem::ProcessStepClimbSmooth(World& world, Entity entity, float deltaTime)
{
	auto& transform = world.GetComponent<TransformComponent>(entity);
	auto& collider = world.GetComponent<VoxelColliderComponent>(entity);

	if (!collider.isClimbing || !collider.smoothStepClimbing)
	{
		return;
	}

	Vector3 currentPos = transform.transition;
	Vector3 targetPos = collider.climbTargetPos;
	Vector3 diff = targetPos - currentPos;
	float distance = diff.Length();

	if (distance < 0.01f)
	{
		transform.transition = targetPos;
		collider.isClimbing = false;
		collider.isGrounded = true;
		return;
	}

	float moveSpeed = collider.stepClimbSpeed * deltaTime;

	if (moveSpeed >= distance)
	{
		transform.transition = targetPos;
		collider.isClimbing = false;
		collider.isGrounded = true;
	}
	else
	{
		Vector3 moveDir = diff.NormalizedCopy();
		transform.transition = currentPos + moveDir * moveSpeed;

		collider.isGrounded = true;
		collider.verticalVelocity = 0.0f;
	}
}

bool VoxelCollisionSystem::TryStepClimbOnPlatform(World& world,
	Entity entity,
	const Vector3& currentPos,
	const Vector3& horizontalMovement,
	const Vector3& halfExtents,
	float maxStepHeight,
	Vector3& outNewPos) const
{
	Vector3 horizontalDir = horizontalMovement;
	horizontalDir.y = 0;
	float horizontalSpeed = horizontalDir.Length();

	if (horizontalSpeed < 0.001f) return false;

	horizontalDir.Normalize();

	auto& registry = world.GetRegistry();
	auto view = registry.view<TransformComponent, DynamicVoxelBodyComponent>();

	float feetY = currentPos.y - halfExtents.y;

	for (auto platformEntity : view)
	{
		if (platformEntity == entity) continue;

		auto& body = view.get<DynamicVoxelBodyComponent>(platformEntity);
		if (!body.isPlatform) continue;

		Vector3 bodyMin, bodyMax;
		const_cast<VoxelCollisionSystem*>(this)->GetDynamicBodyAABB(
			const_cast<World&>(world), platformEntity, bodyMin, bodyMax);

		float platformTopY = bodyMax.y;
		float stepHeight = platformTopY - feetY;

		if (stepHeight < 0.05f || stepHeight > maxStepHeight)
		{
			continue;
		}

		float checkDistance = halfExtents.x + 0.2f;

		Vector3 playerFeetMin = currentPos - halfExtents;
		playerFeetMin.y = feetY;
		Vector3 playerFeetMax = currentPos + halfExtents;
		playerFeetMax.y = feetY + stepHeight;

		playerFeetMin = playerFeetMin + horizontalDir * 0.01f;
		playerFeetMax = playerFeetMax + horizontalDir * checkDistance;

		bool overlapsX = (playerFeetMax.x > bodyMin.x && playerFeetMin.x < bodyMax.x);
		bool overlapsY = (playerFeetMax.y > bodyMin.y && playerFeetMin.y < bodyMax.y);
		bool overlapsZ = (playerFeetMax.z > bodyMin.z && playerFeetMin.z < bodyMax.z);

		if (!overlapsX || !overlapsY || !overlapsZ)
		{
			continue;
		}

		Vector3 targetPos = currentPos + horizontalDir * checkDistance;

		float clampedX = std::max(bodyMin.x + halfExtents.x + 0.01f,
			std::min(bodyMax.x - halfExtents.x - 0.01f, targetPos.x));
		float clampedZ = std::max(bodyMin.z + halfExtents.z + 0.01f,
			std::min(bodyMax.z - halfExtents.z - 0.01f, targetPos.z));

		if (bodyMax.x - bodyMin.x < halfExtents.x * 2.0f + 0.02f)
		{
			clampedX = (bodyMin.x + bodyMax.x) * 0.5f;
		}
		if (bodyMax.z - bodyMin.z < halfExtents.z * 2.0f + 0.02f)
		{
			clampedZ = (bodyMin.z + bodyMax.z) * 0.5f;
		}

		targetPos.x = clampedX;
		targetPos.z = clampedZ;
		targetPos.y = platformTopY + halfExtents.y + kSkinWidth;

		Vector3 targetMin = targetPos - halfExtents;
		Vector3 targetMax = targetPos + halfExtents;

		if (mVoxelWorld && mVoxelWorld->OverlapsSolid(targetMin, targetMax))
		{
			continue;
		}

		bool hasOtherCollision = false;
		for (auto otherEntity : view)
		{
			if (otherEntity == entity || otherEntity == platformEntity) continue;

			Vector3 otherMin, otherMax;
			const_cast<VoxelCollisionSystem*>(this)->GetDynamicBodyAABB(
				const_cast<World&>(world), otherEntity, otherMin, otherMax);

			if (targetMax.x > otherMin.x && targetMin.x < otherMax.x &&
				targetMax.y > otherMin.y && targetMin.y < otherMax.y &&
				targetMax.z > otherMin.z && targetMin.z < otherMax.z)
			{
				hasOtherCollision = true;
				break;
			}
		}

		if (hasOtherCollision)
		{
			continue;
		}

		outNewPos = targetPos;
		return true;
	}

	return false;
}

bool VoxelCollisionSystem::IsPositionValid(const Vector3& position,
	const Vector3& halfExtents) const
{
	if (!mVoxelWorld) return true;

	Vector3 boxMin = position - halfExtents;
	Vector3 boxMax = position + halfExtents;

	return !mVoxelWorld->OverlapsSolid(boxMin, boxMax);
}

bool VoxelCollisionSystem::TryStepClimb(const Vector3& currentPos,
	const Vector3& horizontalMovement,
	const Vector3& halfExtents,
	float maxStepHeight,
	float stepSearchOvershoot,
	float minStepDepth,
	Vector3& outNewPos) const
{
	if (!mVoxelWorld) return false;

	Vector3 horizontalDir = horizontalMovement;
	horizontalDir.y = 0;
	float horizontalSpeed = horizontalDir.Length();

	if (horizontalSpeed < 0.001f) return false;

	horizontalDir.Normalize();

	float feetY = currentPos.y - halfExtents.y;

	float checkDistance = halfExtents.x + stepSearchOvershoot + 0.1f;

	Vector3 perpDir(-horizontalDir.z, 0, horizontalDir.x);

	float detectWidth = halfExtents.x;
	int numCheckPoints = 3;

	bool hasObstacle = false;

	for (int i = 0; i < numCheckPoints; ++i)
	{
		float t = (numCheckPoints > 1) ? (float)i / (float)(numCheckPoints - 1) : 0.5f;
		float offset = (t - 0.5f) * 2.0f * detectWidth;

		Vector3 checkOffset = perpDir * offset;
		Vector3 feetCheckPos = currentPos + horizontalDir * checkDistance + checkOffset;

		Vector3 feetCheckMin = feetCheckPos - Vector3(halfExtents.x * 0.4f, halfExtents.y, halfExtents.z * 0.4f);
		Vector3 feetCheckMax = feetCheckPos + Vector3(halfExtents.x * 0.4f, 0, halfExtents.z * 0.4f);
		feetCheckMax.y = feetY + maxStepHeight;

		if (mVoxelWorld->OverlapsSolid(feetCheckMin, feetCheckMax))
		{
			hasObstacle = true;
			break;
		}
	}

	if (!hasObstacle)
	{
		return false;
	}

	const float stepIncrement = mVoxelWorld->voxelSize * 0.2f;

	for (float stepHeight = stepIncrement; stepHeight <= maxStepHeight + 0.01f; stepHeight += stepIncrement)
	{
		Vector3 raisedPos = currentPos;
		raisedPos.y += stepHeight;

		Vector3 raisedMin = raisedPos - halfExtents;
		Vector3 raisedMax = raisedPos + halfExtents;

		if (mVoxelWorld->OverlapsSolid(raisedMin, raisedMax))
		{
			continue;
		}

		Vector3 forwardPos = raisedPos + horizontalDir * checkDistance;
		Vector3 forwardMin = forwardPos - halfExtents;
		Vector3 forwardMax = forwardPos + halfExtents;

		if (mVoxelWorld->OverlapsSolid(forwardMin, forwardMax))
		{
			continue;
		}

		Vector3 landingPos = forwardPos;
		bool foundGround = false;

		for (float dropY = 0; dropY <= stepHeight + halfExtents.y + 0.5f; dropY += stepIncrement)
		{
			Vector3 testPos = forwardPos;
			testPos.y -= dropY;

			Vector3 testMin = testPos - halfExtents;
			Vector3 testMax = testPos + halfExtents;

			if (mVoxelWorld->OverlapsSolid(testMin, testMax))
			{
				if (dropY > 0)
				{
					landingPos.y = forwardPos.y - (dropY - stepIncrement);
					foundGround = true;
				}
				break;
			}

			Vector3 groundCheckMin = testMin;
			groundCheckMin.y -= 0.2f;
			Vector3 groundCheckMax = testMax;
			groundCheckMax.y = testMin.y + 0.02f;

			if (mVoxelWorld->OverlapsSolid(groundCheckMin, groundCheckMax))
			{
				landingPos = testPos;
				foundGround = true;
				break;
			}
		}

		if (!foundGround)
		{
			continue;
		}

		float actualStepHeight = (landingPos.y - halfExtents.y) - feetY;
		if (actualStepHeight < stepIncrement * 0.25f)
		{
			continue;
		}

		outNewPos = landingPos;
		return true;
	}

	return false;
}
