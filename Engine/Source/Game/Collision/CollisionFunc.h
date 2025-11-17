#pragma once
#include"CollisionCommon.h"

namespace Collision
{
	//球と球の当たり判定
	bool IsCollision(const Sphere& a, const Sphere& b);
	//AABBとAAABの当たり判定
	bool IsCollision(const AABB& a, const AABB& b);
	//AABBと球の当たり判定
	bool IsCollision(const AABB& aabb, const Sphere& sphere);
}
