#pragma once
#include "Model.h"
#include "Animation.h"
#include "Material.h"
#include "Skeleton.h"

#include <map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace AtomEngine
{
    struct ModelData
    {
        AxisAlignedBox boundingBox;

        std::vector<Material> materials;
        std::vector<Mesh> meshes;
        std::vector<AnimationClip> animations;
        Skeleton skeleton;
        std::map<std::string, JointWeightData> skinClusterData;
    };

    class ModelLoader
    {
    public:

        static bool LoadModel(ModelData& model, const std::string& filePath);

    private:
        static void ProcessNode(ModelData& model, aiNode* node, const aiScene* scene);
        static Mesh ProcessMesh(ModelData& model, aiMesh* mesh);
        static void ProcessMaterial(ModelData& model, aiMaterial* mat);
        static void ProcessAnimations(ModelData& model, const aiScene* scene);
        static void ComputeBoundingVolumes(ModelData& model);

    };
}