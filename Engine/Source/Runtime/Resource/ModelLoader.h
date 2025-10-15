#pragma once
#include "Model.h"
#include "Animation.h"
#include "Material.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace AtomEngine
{
    // MaterialConstantsの非整列コピー
    struct MaterialConstantData
    {
        float baseColorFactor[4];   // default=[1,1,1,1]
        float emissiveFactor[3];    // default=[0,0,0]
        float normalTextureScale;   // default=1
        float metallicFactor;       // default=1
        float roughnessFactor;      // default=1
        uint32_t flags;
    };

    // ロード時に記述子テーブルを構築するために使用される
    struct MaterialTextureData
    {
        uint16_t stringIdx[kNumTextures];
        uint32_t addressModes;
    };

    struct ModelData
    {
        BoundingSphere m_BoundingSphere;
        AxisAlignedBox m_BoundingBox;
        std::vector<byte> m_GeometryData;
        std::vector<byte> m_AnimationKeyFrameData;
        std::vector<AnimationCurve> m_AnimationCurves;
        std::vector<AnimationSet> m_Animations;
        std::vector<uint16_t> m_JointIndices;
        std::vector<Matrix4x4> m_JointIBMs;
        std::vector<MaterialTextureData> m_MaterialTextures;
        std::vector<MaterialConstantData> m_MaterialConstants;
        std::vector<std::unique_ptr<Mesh>> m_Meshes;
        std::vector<GraphNode> m_SceneGraph;
        std::vector<std::string> m_TextureNames;
        std::vector<uint8_t> m_TextureOptions;
    };
    
    class ModelLoader
    {
    public:

        static bool LoadModel(ModelData& model, const std::string& filePath);

    private:
        static uint32_t ProcessNode(ModelData& model, const aiScene* scene,const aiNode* node, uint32_t curPos, const Matrix4x4& parentTransform);
        static void ProcessMaterial(ModelData& model, const aiScene* scene);
        static void ProcessAnimations(ModelData& model, const aiScene* scene);
        static void ProcessSkins(ModelData& model, const aiScene* scene);
        static void CompileMesh(ModelData& model, const aiScene* scene,const Matrix4x4& localTransform, BoundingSphere& boundingSphere, AxisAlignedBox& aabb);
    };
}
