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

        ByteAddressBuffer m_GeometryBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW m_IndexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_VertexBufferDepth;
        D3D12_INDEX_BUFFER_VIEW m_IndexBufferDepth;
    };

	class ModelLoader
	{
	public:

        static bool LoadModel(ModelData& model,const std::string& filePath);
    
    private:
        static void ProcessNode(ModelData& model, aiNode* node, const aiScene* scene, const Matrix4x4& parentTransform);
        static std::unique_ptr<Mesh> ProcessMesh(ModelData& model, aiMesh* mesh, const aiScene* scene);
        static void ProcessMaterial(ModelData& model, aiMaterial* mat);
        static void ProcessAnimations(ModelData& model, const aiScene* scene);
        static void ComputeBoundingVolumes(ModelData& model, const aiScene* scene);

        static void BuildGeometryData(ModelData& model, const aiScene* scene);
        static void CreateVertexData(aiMesh* srcMesh, std::vector<uint8_t>& vertexData, uint32_t& vertexStride);
        static void CreateIndexData(aiMesh* srcMesh, std::vector<uint16_t>& indexData);
	};
}
