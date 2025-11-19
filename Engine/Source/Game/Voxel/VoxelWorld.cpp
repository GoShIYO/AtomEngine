#include "VoxelWorld.h"
#define OGT_VOX_IMPLEMENTATION
#include "ogt_vox.h"
#include <fstream>

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
		data.resize(0,0);
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

		worldSize = { (float)model->size_x, (float)model->size_y, (float)model->size_z };

		for (uint32_t z = 0; z < model->size_z; ++z)
		{
			for (uint32_t y = 0; y < model->size_y; ++y)
			{
				for (uint32_t x = 0; x < model->size_x; ++x)
				{
					uint32_t idx = x + y * model->size_x + z * model->size_x * model->size_y;
					uint8_t color_index = model->voxel_data[idx];
					if (color_index == 0) continue;

					float px = (x + 0.5f) - pivot_x;
					float py = (y + 0.5f) - pivot_y;
					float pz = (z + 0.5f) - pivot_z;

					float wx = T.m00 * px + T.m10 * py + T.m20 * pz + T.m30;
					float wy = T.m01 * px + T.m11 * py + T.m21 * pz + T.m31;
					float wz = T.m02 * px + T.m12 * py + T.m22 * pz + T.m32;

					int ix = (int)std::round(wx);
					int iy = (int)std::round(wz);
					int iz = (int)std::round(wy);

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

	sizeX = max_x - min_x + 1;
	sizeY = max_y - min_y + 1;
	sizeZ = max_z - min_z + 1;

	data.resize(sizeX * sizeY * sizeZ, 0);

    for (auto& v : voxelList)
    {
        int idx = Index(v.x - min_x, v.y - min_y, v.z - min_z);
        data[idx] = v.color;
    }
	return true;
}
