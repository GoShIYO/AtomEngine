#include "VoxelWorld.h"
#define OGT_VOX_IMPLEMENTATION
#include "External/vox/ogt_vox.h"
#include <fstream>
#include <cmath>
#include "Runtime/Core/LogSystem/LogSystem.h"
#include <cfloat>
#include <algorithm>

void VoxelWorld::Resize(int w, int h, int d)
{
	width = w; height = h; depth = d;
	data.resize(w * h * d, 0);
}

bool VoxelWorld::Load(const std::string& filename)
{
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file) return false;

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::vector<uint8_t> buffer(size);
	if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) return false;

	const ogt_vox_scene* scene = ogt_vox_read_scene(buffer.data(), (uint32_t)buffer.size());
	if (!scene) return false;

	auto sceneDeleter = [](const ogt_vox_scene* s) { ogt_vox_destroy_scene(s); };
	std::unique_ptr<const ogt_vox_scene, decltype(sceneDeleter)> scenePtr(scene, sceneDeleter);

	if (scene->num_instances == 0)
	{
		Resize(0, 0, 0);
		return true;
	}

	struct VoxelPos { int x, y, z; uint8_t color; };
	std::vector<VoxelPos> voxelList;

	int min_x = INT32_MAX, min_y = INT32_MAX, min_z = INT32_MAX;
	int max_x = INT32_MIN, max_y = INT32_MIN, max_z = INT32_MIN;

	for (uint32_t i = 0; i < scene->num_instances; ++i)
	{
		const ogt_vox_instance& instance = scene->instances[i];
		const ogt_vox_model* model = scene->models[instance.model_index];
		const ogt_vox_transform& T = instance.transform;

		float pivot_x = model->size_x * 0.5f;
		float pivot_y = model->size_y * 0.5f;
		float pivot_z = model->size_z * 0.5f;

		for (uint32_t z = 0; z < model->size_z; ++z)
		{
			for (uint32_t y = 0; y < model->size_y; ++y)
			{
				for (uint32_t x = 0; x < model->size_x; ++x)
				{
					uint32_t idx = x + (y * model->size_x) + (z * model->size_x * model->size_y);
					uint8_t color_index = model->voxel_data[idx];
					if (color_index == 0) continue;

					float px = (x + 0.5f) - pivot_x;
					float py = (y + 0.5f) - pivot_y;
					float pz = (z + 0.5f) - pivot_z;

					float wx = T.m00 * px + T.m10 * py + T.m20 * pz + T.m30;
					float wy = T.m01 * px + T.m11 * py + T.m21 * pz + T.m31;
					float wz = T.m02 * px + T.m12 * py + T.m22 * pz + T.m32;

					int ix = (int)std::floor(wx);
					int iy = (int)std::floor(wz);
					int iz = (int)std::floor(wy);

					voxelList.push_back({ ix, iy, iz, color_index });

					min_x = std::min(min_x, ix);
					min_y = std::min(min_y, iy);
					min_z = std::min(min_z, iz);
					max_x = std::max(max_x, ix);
					max_y = std::max(max_y, iy);
					max_z = std::max(max_z, iz);
				}
			}
		}
	}

	int map_w = (max_x - min_x) + 1;
	int map_h = (max_y - min_y) + 1;
	int map_d = (max_z - min_z) + 1;

	Resize(map_w, map_h, map_d);

	for (auto& v : voxelList)
	{
		int lx = v.x - min_x;
		int ly = v.y - min_y;
		int lz = v.z - min_z;
		Set(lx, ly, lz, v.color);
	}

	worldSize = Vector3((float)width, (float)height, (float)depth) * voxelSize;

	origin = worldSize * -0.5f;
	origin.y = 0.0f;

	return true;
}

std::optional<VoxelWorld::CellCoord> VoxelWorld::WorldToCoord(const Vector3& worldPos) const
{
	Vector3 local = (worldPos - origin) / voxelSize;
	int x = (int)std::floor(local.x);
	int y = (int)std::floor(local.y);
	int z = (int)std::floor(local.z);
	if (!InBounds(x, y, z)) return std::nullopt;
	return CellCoord{ x, y, z };
}

Vector3 VoxelWorld::CoordToWorldCenter(const CellCoord& coord) const
{
	Vector3 c((float)coord.x + 0.5f, (float)coord.y + 0.5f, (float)coord.z + 0.5f);
	return origin + c * voxelSize;
}

Vector3 VoxelWorld::CoordToWorldMin(int x, int y, int z) const
{
	return origin + Vector3((float)x, (float)y, (float)z) * voxelSize;
}

Vector3 VoxelWorld::CoordToWorldMax(int x, int y, int z) const
{
	return origin + Vector3((float)(x + 1), (float)(y + 1), (float)(z + 1)) * voxelSize;
}

bool VoxelWorld::IsSolid(const Vector3& worldPos) const
{
	auto coord = WorldToCoord(worldPos);
	if (!coord) return false;
	return IsSolid(coord->x, coord->y, coord->z);
}

bool VoxelWorld::OverlapsSolid(const Vector3& worldMin, const Vector3& worldMax) const
{
	Vector3 localMin = (worldMin - origin) / voxelSize;
	Vector3 localMax = (worldMax - origin) / voxelSize;

	int minX = (int)std::floor(localMin.x);
	int minY = (int)std::floor(localMin.y);
	int minZ = (int)std::floor(localMin.z);
	int maxX = (int)std::floor(localMax.x);
	int maxY = (int)std::floor(localMax.y);
	int maxZ = (int)std::floor(localMax.z);

	for (int z = minZ; z <= maxZ; ++z)
	{
		for (int y = minY; y <= maxY; ++y)
		{
			for (int x = minX; x <= maxX; ++x)
			{
				if (IsSolid(x, y, z)) return true;
			}
		}
	}
	return false;
}

std::optional<VoxelHit> VoxelWorld::Raycast(const Vector3& rayOrigin, const Vector3& direction, float maxDistance) const
{
	Vector3 dir = direction.NormalizedCopy();
	
	Vector3 localOrigin = (rayOrigin - origin) / voxelSize;
	
	int x = (int)std::floor(localOrigin.x);
	int y = (int)std::floor(localOrigin.y);
	int z = (int)std::floor(localOrigin.z);

	int stepX = (dir.x >= 0) ? 1 : -1;
	int stepY = (dir.y >= 0) ? 1 : -1;
	int stepZ = (dir.z >= 0) ? 1 : -1;
	
	float tMaxX = (dir.x != 0) ? ((stepX > 0 ? (x + 1 - localOrigin.x) : (localOrigin.x - x)) / std::abs(dir.x)) : FLT_MAX;
	float tMaxY = (dir.y != 0) ? ((stepY > 0 ? (y + 1 - localOrigin.y) : (localOrigin.y - y)) / std::abs(dir.y)) : FLT_MAX;
	float tMaxZ = (dir.z != 0) ? ((stepZ > 0 ? (z + 1 - localOrigin.z) : (localOrigin.z - z)) / std::abs(dir.z)) : FLT_MAX;
	
	float tDeltaX = (dir.x != 0) ? (1.0f / std::abs(dir.x)) : FLT_MAX;
	float tDeltaY = (dir.y != 0) ? (1.0f / std::abs(dir.y)) : FLT_MAX;
	float tDeltaZ = (dir.z != 0) ? (1.0f / std::abs(dir.z)) : FLT_MAX;
	
	float maxT = maxDistance / voxelSize;
	float t = 0.0f;
	Vector3 normal = Vector3::ZERO;
	
	if (InBounds(x, y, z) && IsSolid(x, y, z))
	{
		VoxelHit hit;
		hit.position = rayOrigin;
		hit.normal = -dir;
		hit.distance = 0.0f;
		hit.voxelX = x;
		hit.voxelY = y;
		hit.voxelZ = z;
		return hit;
	}
	
	while (t < maxT)
	{
		if (tMaxX < tMaxY && tMaxX < tMaxZ)
		{
			t = tMaxX;
			x += stepX;
			tMaxX += tDeltaX;
			normal = Vector3((float)-stepX, 0.0f, 0.0f);
		}
		else if (tMaxY < tMaxZ)
		{
			t = tMaxY;
			y += stepY;
			tMaxY += tDeltaY;
			normal = Vector3(0.0f, (float)-stepY, 0.0f);
		}
		else
		{
			t = tMaxZ;
			z += stepZ;
			tMaxZ += tDeltaZ;
			normal = Vector3(0.0f, 0.0f, (float)-stepZ);
		}
		
		if (!InBounds(x, y, z)) break;
		
		if (IsSolid(x, y, z))
		{
			VoxelHit hit;
			hit.distance = t * voxelSize;
			hit.position = rayOrigin + dir * hit.distance;
			hit.normal = normal;
			hit.voxelX = x;
			hit.voxelY = y;
			hit.voxelZ = z;
			return hit;
		}
	}
	
	return std::nullopt;
}

std::vector<VoxelWorld::CellCoord> VoxelWorld::GetOverlappingVoxels(const Vector3& worldMin, const Vector3& worldMax) const
{
	std::vector<CellCoord> result;
	
	Vector3 localMin = (worldMin - origin) / voxelSize;
	Vector3 localMax = (worldMax - origin) / voxelSize;
	
	int minX = (int)std::floor(localMin.x);
	int minY = (int)std::floor(localMin.y);
	int minZ = (int)std::floor(localMin.z);
	int maxX = (int)std::floor(localMax.x);
	int maxY = (int)std::floor(localMax.y);
	int maxZ = (int)std::floor(localMax.z);
	
	for (int z = minZ; z <= maxZ; ++z)
	{
		for (int y = minY; y <= maxY; ++y)
		{
			for (int x = minX; x <= maxX; ++x)
			{
				if (IsSolid(x, y, z))
				{
					result.push_back({ x, y, z });
				}
			}
		}
	}
	
	return result;
}

VoxelCollisionResult VoxelWorld::ResolveAABBCollision(const Vector3& boxMin, const Vector3& boxMax) const
{
	VoxelCollisionResult result;
	result.correctedPosition = (boxMin + boxMax) * 0.5f;
	
	auto overlapping = GetOverlappingVoxels(boxMin, boxMax);
	if (overlapping.empty())
	{
		return result;
	}
	
	result.collided = true;

	float minPenetration = FLT_MAX;
	Vector3 separationNormal = Vector3::ZERO;
	float totalPenetration = 0.0f;
	
	for (const auto& voxel : overlapping)
	{
		Vector3 voxelMin = CoordToWorldMin(voxel.x, voxel.y, voxel.z);
		Vector3 voxelMax = CoordToWorldMax(voxel.x, voxel.y, voxel.z);

		float penetrationX1 = voxelMax.x - boxMin.x;
		float penetrationX2 = boxMax.x - voxelMin.x;
		float penetrationY1 = voxelMax.y - boxMin.y;
		float penetrationY2 = boxMax.y - voxelMin.y;
		float penetrationZ1 = voxelMax.z - boxMin.z;
		float penetrationZ2 = boxMax.z - voxelMin.z;

		struct AxisPenetration
		{
			float depth;
			Vector3 normal;
		};
		
		AxisPenetration axes[6] = {
			{ penetrationX1, Vector3(1.0f, 0.0f, 0.0f) },
			{ penetrationX2, Vector3(-1.0f, 0.0f, 0.0f) },
			{ penetrationY1, Vector3(0.0f, 1.0f, 0.0f) },
			{ penetrationY2, Vector3(0.0f, -1.0f, 0.0f) },
			{ penetrationZ1, Vector3(0.0f, 0.0f, 1.0f) },
			{ penetrationZ2, Vector3(0.0f, 0.0f, -1.0f) }
		};
		
		for (int i = 0; i < 6; ++i)
		{
			if (axes[i].depth > 0 && axes[i].depth < minPenetration)
			{
				minPenetration = axes[i].depth;
				separationNormal = axes[i].normal;
			}
		}
	}
	
	if (minPenetration < FLT_MAX && minPenetration > 0.0f)
	{
		result.penetration = separationNormal * minPenetration;
		result.normal = separationNormal;
		result.correctedPosition = result.correctedPosition + result.penetration;
	}
	
	return result;
}

VoxelSweepResult VoxelWorld::SweepAABB(const Vector3& boxMin, const Vector3& boxMax, const Vector3& velocity) const
{
	VoxelSweepResult result;
	result.time = 1.0f;
	result.hit = false;
	
	float velocityLength = velocity.Length();
	if (velocityLength < 0.0001f)
	{
		return result;
	}
	
	Vector3 sweepMin = Vector3(
		std::min(boxMin.x, boxMin.x + velocity.x),
		std::min(boxMin.y, boxMin.y + velocity.y),
		std::min(boxMin.z, boxMin.z + velocity.z)
	);
	Vector3 sweepMax = Vector3(
		std::max(boxMax.x, boxMax.x + velocity.x),
		std::max(boxMax.y, boxMax.y + velocity.y),
		std::max(boxMax.z, boxMax.z + velocity.z)
	);

	Vector3 localMin = (sweepMin - origin) / voxelSize;
	Vector3 localMax = (sweepMax - origin) / voxelSize;
	
	int minX = (int)std::floor(localMin.x);
	int minY = (int)std::floor(localMin.y);
	int minZ = (int)std::floor(localMin.z);
	int maxX = (int)std::floor(localMax.x);
	int maxY = (int)std::floor(localMax.y);
	int maxZ = (int)std::floor(localMax.z);
	
	float earliestTime = 1.0f;
	Vector3 hitNormal = Vector3::ZERO;
	int hitVoxelX = 0, hitVoxelY = 0, hitVoxelZ = 0;
	
	for (int z = minZ; z <= maxZ; ++z)
	{
		for (int y = minY; y <= maxY; ++y)
		{
			for (int x = minX; x <= maxX; ++x)
			{
				if (!IsSolid(x, y, z)) continue;
				
				Vector3 voxelMin = CoordToWorldMin(x, y, z);
				Vector3 voxelMax = CoordToWorldMax(x, y, z);
				
				float tEntry = 0.0f;
				float tExit = 1.0f;
				Vector3 entryNormal = Vector3::ZERO;
				bool separated = false;
				
				// X轴
				if (velocity.x != 0.0f)
				{
					float invVelX = 1.0f / velocity.x;
					float t1 = (voxelMin.x - boxMax.x) * invVelX;
					float t2 = (voxelMax.x - boxMin.x) * invVelX;
					
					float tEntryX = std::min(t1, t2);
					float tExitX = std::max(t1, t2);
					
					if (tEntryX > tEntry)
					{
						tEntry = tEntryX;
						entryNormal = (velocity.x > 0) ? Vector3(-1, 0, 0) : Vector3(1, 0, 0);
					}
					tExit = std::min(tExit, tExitX);
				}
				else
				{
					if (boxMax.x < voxelMin.x || boxMin.x > voxelMax.x)
						separated = true;
				}
				
				// Y轴
				if (!separated && velocity.y != 0.0f)
				{
					float invVelY = 1.0f / velocity.y;
					float t1 = (voxelMin.y - boxMax.y) * invVelY;
					float t2 = (voxelMax.y - boxMin.y) * invVelY;
					
					float tEntryY = std::min(t1, t2);
					float tExitY = std::max(t1, t2);
					
					if (tEntryY > tEntry)
					{
						tEntry = tEntryY;
						entryNormal = (velocity.y > 0) ? Vector3(0, -1, 0) : Vector3(0, 1, 0);
					}
					tExit = std::min(tExit, tExitY);
				}
				else if (!separated)
				{
					if (boxMax.y < voxelMin.y || boxMin.y > voxelMax.y)
						separated = true;
				}
				
				// Z轴
				if (!separated && velocity.z != 0.0f)
				{
					float invVelZ = 1.0f / velocity.z;
					float t1 = (voxelMin.z - boxMax.z) * invVelZ;
					float t2 = (voxelMax.z - boxMin.z) * invVelZ;
					
					float tEntryZ = std::min(t1, t2);
					float tExitZ = std::max(t1, t2);
					
					if (tEntryZ > tEntry)
					{
						tEntry = tEntryZ;
						entryNormal = (velocity.z > 0) ? Vector3(0, 0, -1) : Vector3(0, 0, 1);
					}
					tExit = std::min(tExit, tExitZ);
				}
				else if (!separated)
				{
					if (boxMax.z < voxelMin.z || boxMin.z > voxelMax.z)
						separated = true;
				}

				if (!separated && tEntry < tExit && tEntry >= 0.0f && tEntry < earliestTime)
				{
					earliestTime = tEntry;
					hitNormal = entryNormal;
					hitVoxelX = x;
					hitVoxelY = y;
					hitVoxelZ = z;
				}
			}
		}
	}
	
	if (earliestTime < 1.0f)
	{
		result.hit = true;
		result.time = std::max(0.0f, earliestTime - 0.001f);
		result.normal = hitNormal;
		result.contactPoint = (boxMin + boxMax) * 0.5f + velocity * result.time;
		result.voxelX = hitVoxelX;
		result.voxelY = hitVoxelY;
		result.voxelZ = hitVoxelZ;
	}
	
	return result;
}

Vector3 VoxelWorld::MoveAndSlide(const Vector3& position, const Vector3& halfExtents,
								  const Vector3& velocity, int maxIterations) const
{
	Vector3 currentPos = position;
	Vector3 remainingVelocity = velocity;
	
	for (int i = 0; i < maxIterations && remainingVelocity.LengthSqr() > 0.0001f; ++i)
	{
		Vector3 boxMin = currentPos - halfExtents;
		Vector3 boxMax = currentPos + halfExtents;
		
		VoxelSweepResult sweep = SweepAABB(boxMin, boxMax, remainingVelocity);
		
		if (!sweep.hit)
		{
			currentPos += remainingVelocity;
			break;
		}
		
		currentPos += remainingVelocity * sweep.time;

		float remainingTime = 1.0f - sweep.time;
		Vector3 slideVelocity = remainingVelocity * remainingTime;

		float normalComponent = slideVelocity.Dot(sweep.normal);
		slideVelocity = slideVelocity - sweep.normal * normalComponent;
		
		remainingVelocity = slideVelocity;
	}

	Vector3 boxMin = currentPos - halfExtents;
	Vector3 boxMax = currentPos + halfExtents;
	VoxelCollisionResult collision = ResolveAABBCollision(boxMin, boxMax);
	
	if (collision.collided)
	{
		currentPos = collision.correctedPosition;
	}
	
	return currentPos;
}

bool VoxelWorld::IsGrounded(const Vector3& position, const Vector3& halfExtents, float groundCheckDistance) const
{
	Vector3 checkMin = position - halfExtents;
	checkMin.y -= groundCheckDistance;
	Vector3 checkMax = position + halfExtents;
	checkMax.y = position.y - halfExtents.y + 0.01f;
	
	return OverlapsSolid(checkMin, checkMax);
}

float VoxelWorld::SignedDistanceToVoxel(const Vector3& point, int vx, int vy, int vz) const
{
	Vector3 voxelMin = CoordToWorldMin(vx, vy, vz);
	Vector3 voxelMax = CoordToWorldMax(vx, vy, vz);
	Vector3 center = (voxelMin + voxelMax) * 0.5f;
	Vector3 halfSize = (voxelMax - voxelMin) * 0.5f;
	
	Vector3 d = Vector3(
		std::abs(point.x - center.x),
		std::abs(point.y - center.y),
		std::abs(point.z - center.z)
	) - halfSize;
	
	Vector3 dClamped = Vector3(
		std::max(d.x, 0.0f),
		std::max(d.y, 0.0f),
		std::max(d.z, 0.0f)
	);
	
	return dClamped.Length() + std::min(std::max(d.x, std::max(d.y, d.z)), 0.0f);
}

Vector3 VoxelWorld::GetVoxelNormal(const Vector3& point, int vx, int vy, int vz) const
{
	Vector3 voxelMin = CoordToWorldMin(vx, vy, vz);
	Vector3 voxelMax = CoordToWorldMax(vx, vy, vz);
	Vector3 center = (voxelMin + voxelMax) * 0.5f;
	
	Vector3 diff = point - center;
	Vector3 absDiff = Vector3(std::abs(diff.x), std::abs(diff.y), std::abs(diff.z));

	if (absDiff.x > absDiff.y && absDiff.x > absDiff.z)
		return Vector3(diff.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
	else if (absDiff.y > absDiff.z)
		return Vector3(0.0f, diff.y > 0 ? 1.0f : -1.0f, 0.0f);
	else
		return Vector3(0.0f, 0.0f, diff.z > 0 ? 1.0f : -1.0f);
}

