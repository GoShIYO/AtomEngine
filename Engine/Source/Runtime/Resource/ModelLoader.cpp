#include "ModelLoader.h"
#include <map>

namespace AtomEngine
{

	struct TextureMapping
	{
		aiTextureType aiType;
		TextureSlot slot;
	};

	static const TextureMapping textureMap[] =
	{
		{ aiTextureType_BASE_COLOR,					TextureSlot::kBaseColor			},
		{ aiTextureType_DIFFUSE,					TextureSlot::kBaseColor			},
		{ aiTextureType_NORMALS,					TextureSlot::kNormal			},
		{ aiTextureType_HEIGHT,						TextureSlot::kNormal			},
		{ aiTextureType_LIGHTMAP,					TextureSlot::kOcclusion			},
		{ aiTextureType_AMBIENT_OCCLUSION,			TextureSlot::kOcclusion			},
		{ aiTextureType_EMISSIVE,					TextureSlot::kEmissive			}
	};

	bool SetTextures(Material& material, aiMaterial* mat, aiTextureType type, TextureSlot slot)
	{
		aiString texPath;
		if (mat->GetTexture(type, 0, &texPath) == AI_SUCCESS && texPath.length > 0)
		{
			MaterialTexture texData;
			texData.name = UTF8ToWString(texPath.C_Str());
			texData.slot = slot;

			material.textures.push_back(std::move(texData));
			material.textureMask |= (1 << static_cast<uint8_t>(slot));
			return true;
		}
		return false;
	}

	Matrix4x4 ConvertTransform(const aiMatrix4x4& matrix)
	{
		aiVector3D scale, translate;
		aiQuaternion rotate;
		matrix.Decompose(scale, rotate, translate);

		Transform transform;
		transform.scale = Vector3(scale.x, scale.y, scale.z);
		transform.rotation = Quaternion(rotate.x, rotate.y, rotate.z, rotate.w);
		transform.transition = Vector3(translate.x, translate.y, translate.z);

		return transform.GetMatrix();
	}


	bool ModelLoader::LoadModel(ModelData& model, const std::string& filePath)
	{
		uint32_t flag =
			aiProcess_MakeLeftHanded |
			aiProcess_FlipWindingOrder |
			aiProcess_Triangulate |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_SplitLargeMeshes |
			aiProcess_CalcTangentSpace |
			aiProcess_GenSmoothNormals |
			aiProcess_JoinIdenticalVertices |
			aiProcess_LimitBoneWeights |
			aiProcess_GenUVCoords;

		Assimp::Importer importer;
		//いらない情報を削除
		importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
			aiComponent_COLORS | aiComponent_LIGHTS | aiComponent_CAMERAS);
		importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, INT_MAX);
		importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 0xfffe);
		importer.SetPropertyFloat(AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE, 45.0f);

		const aiScene* scene = importer.ReadFile(filePath, flag);
		ASSERT(scene && scene->mRootNode && scene->HasMeshes(),"Load Model Failed to scene");
		
		for (uint32_t i = 0; i < scene->mNumMaterials; i++)
			ProcessMaterial(model, scene->mMaterials[i]);

		ProcessNode(model,scene->mRootNode, scene);

		if(scene->HasAnimations())
            ProcessAnimations(model, scene);

		ComputeBoundingVolumes(model);

		return true;
	}

	void ModelLoader::ProcessNode(ModelData& model, aiNode* node, const aiScene* scene)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			
			model.meshes.push_back(ProcessMesh(model, mesh));
		}

		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
            ProcessNode(model, node->mChildren[i], scene);
		}
	}

	Mesh ModelLoader::ProcessMesh(ModelData& model, aiMesh* mesh)
	{
		Mesh outMesh;
		outMesh.vertexCount = mesh->mNumVertices;
		outMesh.indexCount = mesh->mNumFaces * 3;
		outMesh.stride = sizeof(Vertex);

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		vertices.reserve(outMesh.vertexCount);
		indices.reserve(outMesh.indexCount);

		AxisAlignedBox aabb;
		for (uint32_t i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex v = {};
			v.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

			if (mesh->mTextureCoords[0])
				v.texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
			if (mesh->mNormals)
				v.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
			if (mesh->mTangents)
			{
				v.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
				v.bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
			}

			aabb.AddPoint(v.position);
			vertices.push_back(v);
		}

		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			assert(face.mNumIndices == 3);
			for (uint32_t j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		if (mesh->HasBones())
		{
			for (uint32_t b = 0; b < mesh->mNumBones; ++b)
			{
				aiBone* bone = mesh->mBones[b];
				std::string jointName = bone->mName.C_Str();
				JointWeightData& jwd = model.skinClusterData[jointName];

				// inverse bind pose
				aiMatrix4x4 m = bone->mOffsetMatrix;
				aiVector3D s, t; aiQuaternion r;
				m.Decompose(s, r, t);
				jwd.inverseBindPoseMatrix.MakeAffine(
					{ s.x,s.y,s.z }, { r.x,r.y,r.z,r.w }, { t.x,t.y,t.z });

				for (uint32_t w = 0; w < bone->mNumWeights; ++w)
				{
					uint32_t localId = bone->mWeights[w].mVertexId;
					float weight = bone->mWeights[w].mWeight;
					jwd.vertexWeights.push_back({ weight, localId });
				}
			}
		}

		outMesh.boundingBox = aabb;
		SubMesh sub{};
		sub.indexOffset = 0;
		sub.indexCount = outMesh.indexCount;
		sub.vertexOffset = 0;
		sub.vertexCount = outMesh.vertexCount;
		sub.materialIndex = mesh->mMaterialIndex;
		sub.bounds = aabb;
		outMesh.subMeshes.push_back(sub);

		return outMesh;
	}

	void ModelLoader::ProcessMaterial(ModelData& model, aiMaterial* mat)
	{
		Material material = {};
		aiString name;
		if (AI_SUCCESS == mat->Get(AI_MATKEY_NAME, name))
			material.name = name.C_Str();

		aiColor4D baseColor;
		if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor))
		{
			material.baseColorFactor[0] = baseColor.r;
			material.baseColorFactor[1] = baseColor.g;
			material.baseColorFactor[2] = baseColor.b;
			material.baseColorFactor[3] = baseColor.a;
		}

		float metallic = 0.0f, roughness = 0.5f;
		mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
		mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
		material.metallicFactor = metallic;
		material.roughnessFactor = roughness;

		for (auto& entry : textureMap)
			SetTextures(material,mat,entry.aiType, entry.slot);

		if (!SetTextures(material, mat, aiTextureType_GLTF_METALLIC_ROUGHNESS, TextureSlot::kMetallicRoughness))
		{
			SetTextures(material, mat, aiTextureType_METALNESS, TextureSlot::kMetallic);
			SetTextures(material, mat, aiTextureType_DIFFUSE_ROUGHNESS, TextureSlot::kRoughness);
		}

		model.materials.push_back(std::move(material));
	}

	void ModelLoader::ProcessAnimations(ModelData& model, const aiScene* scene)
	{
		for (uint32_t i = 0; i < scene->mNumAnimations; ++i)
		{
			const aiAnimation* anim = scene->mAnimations[i];
			AnimationClip clip{};
			clip.name = anim->mName.C_Str();
			clip.duration = (float)anim->mDuration / (float)anim->mTicksPerSecond;

			for (uint32_t c = 0; c < anim->mNumChannels; ++c)
			{
				const aiNodeAnim* channel = anim->mChannels[c];
				AnimationCurve curve{};
				curve.targetJoint = model.skeleton.jointMap[channel->mNodeName.C_Str()];

				for (uint32_t k = 0; k < channel->mNumPositionKeys; ++k)
				{
					auto& key = channel->mPositionKeys[k];
					curve.translation.push_back({ float(key.mTime / anim->mTicksPerSecond),
						{ key.mValue.x,key.mValue.y,key.mValue.z } });
				}

				for (uint32_t k = 0; k < channel->mNumRotationKeys; ++k)
				{
					auto& key = channel->mRotationKeys[k];
					curve.rotation.push_back({ float(key.mTime / anim->mTicksPerSecond),
						{ key.mValue.x,key.mValue.y,key.mValue.z,key.mValue.w } });
				}

				for (uint32_t k = 0; k < channel->mNumScalingKeys; ++k)
				{
					auto& key = channel->mRotationKeys[k];
					curve.scale.push_back({ float(key.mTime / anim->mTicksPerSecond),
						{ key.mValue.x,key.mValue.y,key.mValue.z } });
				}

				clip.curves.push_back(std::move(curve));
			}

			model.animations.push_back(std::move(clip));
		}
	}

	void ModelLoader::ComputeBoundingVolumes(ModelData& model)
	{
		AxisAlignedBox modelBounds;
		for (auto& mesh : model.meshes)
		{
			modelBounds.AddBoundingBox(mesh.boundingBox);
		}

		model.boundingBox = modelBounds;
	}
	
}
