#pragma once
#include "Runtime/Core/Math/MathInclude.h"

namespace Collision
{
	using namespace AtomEngine;
	struct Sphere
	{
		Sphere() = default;
		Sphere(const Vector3& center_, float radius_)
			: center(center_), radius(radius_)
		{
		}
		Vector3 center;
		float radius;
	};

	struct AABB
	{
		Vector3 min;
		Vector3 max;

		AABB()
		{
			Reset();
		}

		AABB(const Vector3& minC, const Vector3& maxC)
			: min(minC), max(maxC)
		{
		}

		void Reset()
		{
			min = { +std::numeric_limits<float>::max(),
						  +std::numeric_limits<float>::max(),
						  +std::numeric_limits<float>::max() };

			max = { -std::numeric_limits<float>::max(),
						  -std::numeric_limits<float>::max(),
						  -std::numeric_limits<float>::max() };
		}

		Vector3 Size() const
		{
			return max - min;
		}

		Vector3 Center() const
		{
			return (max + min) * 0.5f;
		}
		//表面全部の面積
		float SurfaceArea() const
		{
			Vector3 s = Size();
			return 2.0f * (s.x * s.y + s.x * s.z + s.y * s.z);
		}
		bool Contains(const AABB& other)const
		{
            return (min.x <= other.min.x && other.max.x <= max.x) &&
                   (min.y <= other.min.y && other.max.y <= max.y) &&
                   (min.z <= other.min.z && other.max.z <= max.z);
		}
		AABB Union(const AABB& box)const
		{
			return AABB(Math::Min(min, box.min), Math::Max(max, box.max));
		}
		Vector3 GetDimensions() const { return Math::Max(max - min, Vector3::ZERO); }
		float GetRadius() const { return GetDimensions().Length() * 0.5f; }
	};
}
