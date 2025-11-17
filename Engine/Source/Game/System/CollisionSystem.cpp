#include "CollisionSystem.h"
#include "../Collision/CollisionFunc.h"
#include "../Component/ColliderComponent.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include <stack>

using namespace AtomEngine;

CollisionSystem::CollisionSystem(AtomEngine::World& world)
 : mWorld(world){

	world.GetDispatcher().sink<ComponentAddedEvent>().connect<&CollisionSystem::OnColliderAdded>(this);
	world.GetDispatcher().sink<ComponentRemovedEvent>().connect<&CollisionSystem::OnColliderRemoved>(this);
}

void CollisionSystem::Update()
{
	UpdateAABBCollider();
	UpdateSphereCollider();

	std::vector<std::pair<Entity, Entity>> pairs;

	//broad phase
	BroadPhase(pairs);

	//narrow phase
	NarrowPhase(pairs);
}

void CollisionSystem::OnColliderAdded(const ComponentAddedEvent& ev)
{
	if (ev.type == std::type_index(typeid(AABBCollider)))
	{
		Entity e = ev.entity;
		GameObject* obj = mWorld.GetGameObject(e);
		auto& collider = mWorld.GetComponent<AABBCollider>(e);
		mBVH.Insert(obj, collider.bound);
	}
	else if (ev.type == std::type_index(typeid(SphereCollider)))
	{
		Entity e = ev.entity;
		GameObject* obj = mWorld.GetGameObject(e);
		auto sph = mWorld.GetComponent<SphereCollider>(e).bound;
		Collision::AABB aabb;
		aabb.min = sph.center - Vector3(sph.radius);
		aabb.max = sph.center + Vector3(sph.radius);
		mBVH.Insert(obj, aabb);
	}
}

void CollisionSystem::OnColliderRemoved(const ComponentRemovedEvent& ev)
{
	if (ev.type == std::type_index(typeid(AABBCollider))
		|| ev.type == std::type_index(typeid(SphereCollider)))
	{
		Entity e = ev.entity;
		GameObject* obj = mWorld.GetGameObject(e);
		if (!obj) return;
		BVHNode* leaf = mBVH.FindLeaf(obj);
		if (leaf) mBVH.Remove(leaf);
	}
}

void CollisionSystem::UpdateAABBCollider()
{
	auto view = mWorld.View<AABBCollider, TransformComponent>();

	for (auto entity : view)
	{
		auto& collider = view.get<AABBCollider>(entity);
		auto& transform = view.get<TransformComponent>(entity);

		auto size = transform.GetMatrix().GetScale() * (collider.bound.max - collider.bound.min);
		const auto& center = transform.GetMatrix().GetTrans();
		collider.bound.min = center - size * 0.5f;
		collider.bound.max = center + size * 0.5f;

		GameObject* obj = mWorld.GetGameObject(entity);
		mBVH.Update(obj, collider.bound);
	}

}

void CollisionSystem::UpdateSphereCollider()
{
	auto view = mWorld.View<SphereCollider, TransformComponent>();

	for (auto entity : view)
	{
		auto& collider = view.get<SphereCollider>(entity);
		auto& transform = view.get<TransformComponent>(entity);

		const auto& s = transform.GetMatrix().GetScale();
		float scale = Math::Max(Math::Max(s.x, s.y), s.z);
		float radius = collider.bound.radius * scale;
		const auto& center = transform.GetMatrix().GetTrans();
		collider.bound.center = center;
		collider.bound.radius = radius;

		Collision::AABB aabb;
		aabb.min = center - Vector3(radius);
        aabb.max = center + Vector3(radius);
		GameObject* obj = mWorld.GetGameObject(entity);
		mBVH.Update(obj, aabb);
	}
}

void CollisionSystem::BroadPhase(std::vector<std::pair<Entity, Entity>>& pairs)
{
	if (!mBVH.Root()) return;

	std::stack<BVHNode*> stack;
	stack.push(mBVH.Root());

	while (!stack.empty())
	{
		BVHNode* node = stack.top();
		stack.pop();

		if (node->IsLeaf()) continue;

		BVHNode* left = node->mLeft;
		BVHNode* right = node->mRight;

		if (left && right && Collision::IsCollision(left->mBound, right->mBound))
		{
			CollectPairs(left, right, pairs);
		}

		if (left) stack.push(left);
		if (right) stack.push(right);
	}
}

void CollisionSystem::CollectPairs(BVHNode* a, BVHNode* b,
	std::vector<std::pair<Entity, Entity>>& pairs)
{
	if (!a || !b) return;

	if (a->IsLeaf() && b->IsLeaf())
	{
		pairs.emplace_back(a->mObject->GetHandle(),
			b->mObject->GetHandle());
		return;
	}

	if (a->IsLeaf())
	{
		CollectPairs(a, b->mLeft, pairs);
		CollectPairs(a, b->mRight, pairs);
	}
	else if (b->IsLeaf())
	{
		CollectPairs(a->mLeft, b, pairs);
		CollectPairs(a->mRight, b, pairs);
	}
	else
	{
		CollectPairs(a->mLeft, b->mLeft, pairs);
		CollectPairs(a->mLeft, b->mRight, pairs);
		CollectPairs(a->mRight, b->mLeft, pairs);
		CollectPairs(a->mRight, b->mRight, pairs);
	}
}

void CollisionSystem::NarrowPhase(const std::vector<std::pair<Entity, Entity>>& pairs)
{
	auto& dispatcher = mWorld.GetDispatcher();

	for (auto& p : pairs)
	{
		Entity a = p.first;
		Entity b = p.second;

		bool hit = false;

		if (mWorld.HasComponent<AABBCollider>(a) && mWorld.HasComponent<AABBCollider>(b))
		{
			hit = Collision::IsCollision(
				mWorld.GetComponent<AABBCollider>(a).bound,
				mWorld.GetComponent<AABBCollider>(b).bound
			);
		}
		else if (mWorld.HasComponent<SphereCollider>(a) && mWorld.HasComponent<SphereCollider>(b))
		{
			hit = Collision::IsCollision(
				mWorld.GetComponent<SphereCollider>(a).bound,
				mWorld.GetComponent<SphereCollider>(b).bound
			);
		}
		else
		{
			// AABB vs Sphere
			Collision::AABB aabb;
			Collision::Sphere sph;

			if (mWorld.HasComponent<AABBCollider>(a)) aabb = mWorld.GetComponent<AABBCollider>(a).bound;
			if (mWorld.HasComponent<AABBCollider>(b)) aabb = mWorld.GetComponent<AABBCollider>(b).bound;

			if (mWorld.HasComponent<SphereCollider>(a)) sph = mWorld.GetComponent<SphereCollider>(a).bound;
			if (mWorld.HasComponent<SphereCollider>(b)) sph = mWorld.GetComponent<SphereCollider>(b).bound;

			hit = Collision::IsCollision(aabb, sph);
		}

		if (hit)
			dispatcher.trigger<CollisionEvent>({ a, b });
	}
}