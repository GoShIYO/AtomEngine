#pragma once
#include "Runtime/Function/Framework/ECS/World.h"
#include "../Collision/BVHNode.h"

struct CollisionEvent
{
	AtomEngine::Entity a;
	AtomEngine::Entity b;
};

class CollisionSystem
{
public:
	CollisionSystem(AtomEngine::World& world);
	void Update();

private:
	AtomEngine::DynamicBVH mBVH;
	AtomEngine::World& mWorld;
private:
	void UpdateAABBCollider();
	void UpdateSphereCollider();

	void OnColliderAdded(const AtomEngine::ComponentAddedEvent& ev);
	void OnColliderRemoved(const AtomEngine::ComponentRemovedEvent& ev);
	void BroadPhase(std::vector<std::pair<AtomEngine::Entity, AtomEngine::Entity>>& pairs);

	void CollectPairs(
		AtomEngine::BVHNode* a, 
		AtomEngine::BVHNode* b, 
		std::vector<std::pair<AtomEngine::Entity, AtomEngine::Entity>>& pairs);

	void NarrowPhase(const std::vector<std::pair<AtomEngine::Entity, AtomEngine::Entity>>& pairs);
};

