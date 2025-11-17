#pragma once
#include <variant>
#include "../Collision/CollisionCommon.h"

struct AABBCollider
{
	AABBCollider(const AtomEngine::Vector3& center_, const AtomEngine::Vector3& size_,bool isTrigger_)
		: bound(center_,size_), isTrigger(isTrigger_)
	{
	}
	Collision::AABB bound;
	bool isTrigger;
};

struct SphereCollider
{
	SphereCollider(const AtomEngine::Vector3& center_, float radius_,bool isTrigger_)
		: bound(center_,radius_), isTrigger(isTrigger_)
	{
	}
	Collision::Sphere bound;
	bool isTrigger;
};