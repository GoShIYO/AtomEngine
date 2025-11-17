#include "CollisionFunc.h"
#include<array>
#include<algorithm>

using namespace AtomEngine;

namespace Collision
{
	bool IsCollision(const Sphere& a, const Sphere& b)
	{
		float lengthSq = (a.center - b.center).LengthSqr();
		float radiusSum = a.radius + b.radius;

		return lengthSq < radiusSum * radiusSum;
	}

	bool IsCollision(const AABB& a, const AABB& b)
	{

		float xMin = std::max(a.min.x, b.min.x);
		float xMax = std::min(a.max.x, b.max.x);

		float yMin = std::max(a.min.y, b.min.y);
		float yMax = std::min(a.max.y, b.max.y);

		float zMin = std::max(a.min.z, b.min.z);
		float zMax = std::min(a.max.z, b.max.z);

		return (xMax >= xMin && yMax >= yMin && zMax >= zMin);
	}

	bool IsCollision(const AABB& aabb, const Sphere& sphere)
	{
		// AABBと球の最近接点を求める
		Vector3 closetPoint = {
			std::clamp(sphere.center.x, aabb.min.x, aabb.max.x),
			std::clamp(sphere.center.y, aabb.min.y, aabb.max.y),
			std::clamp(sphere.center.z, aabb.min.z, aabb.max.z)
		};
		// 最近接点と球の中心との距離を求める
		float distance = (closetPoint - sphere.center).LengthSqr();

		return distance <= sphere.radius * sphere.radius;
	}
}

