#include "ModelLoader.h"
#include <map>

namespace AtomEngine
{

	std::unordered_map<std::string, uint32_t> sNodeNameToIndex;

	Transform ConvertTransform(const aiMatrix4x4& matrix)
	{
		aiVector3D scale, translate;
		aiQuaternion rotate;
		matrix.Decompose(scale, rotate, translate);

		Transform transform;
		transform.scale = Vector3(scale.x, scale.y, scale.z);
		transform.rotation = Quaternion(rotate.x, rotate.y, rotate.z, rotate.w);
		transform.transition = Vector3(translate.x, translate.y, translate.z);

		return transform;
	}

	uint32_t FindNodeIndex(const std::string& name)
	{
		auto it = sNodeNameToIndex.find(name);
		if (it != sNodeNameToIndex.end())
			return it->second;
		return 0xFFFFFFFF;
	}

	uint32_t CalculateVertexStride(uint16_t psoFlags)
	{
		uint32_t stride = 0;
		stride += 12;
		if (psoFlags & PSOFlags::kHasNormal) stride += 12;
		if (psoFlags & PSOFlags::kHasTangent) stride += 12;
		if (psoFlags & PSOFlags::kHasUV0) stride += 8;
		if (psoFlags & PSOFlags::kHasUV1) stride += 8;
		return stride;
	}

	void AppendVec2(std::vector<byte>& buffer, const aiVector3D& v)
	{
		float data[2] = { v.x, v.y };
		buffer.insert(buffer.end(),
			reinterpret_cast<byte*>(data),
			reinterpret_cast<byte*>(data + 2));
	}

	void AppendVec2(std::vector<byte>& buffer, const aiVector2D& v)
	{
		float data[2] = { v.x, v.y };
		buffer.insert(buffer.end(),
			reinterpret_cast<byte*>(data),
			reinterpret_cast<byte*>(data + 2));
	}
	void AppendVec3(std::vector<byte>& buffer, const aiVector3D& v)
	{
		float data[3] = { v.x, v.y, v.z };
		buffer.insert(buffer.end(), reinterpret_cast<byte*>(data), reinterpret_cast<byte*>(data + 3));
	}

	void AppendQuat(std::vector<byte>& buffer, const aiQuaternion& q)
	{
		float data[4] = { q.x, q.y, q.z, q.w };
		buffer.insert(buffer.end(), reinterpret_cast<byte*>(data), reinterpret_cast<byte*>(data + 4));
	}

	void LoadTexture(ModelData& model, MaterialTextureData& textureData, const aiMaterial* mat, aiTextureType type, TextureType slot, bool useAlphaTest = false)
	{
		assert(mat);

		aiString path;
		if (mat->GetTexture(type, 0, &path) == AI_SUCCESS)
		{
			textureData.stringIdx[slot] = (uint16_t)(model.m_TextureNames.size() - 1);
			textureData.addressModes = 0x55555;

			switch (type)
			{
			case aiTextureType_BASE_COLOR:
				model.m_TextureOptions.push_back(TextureOptions(true, useAlphaTest));
				break;
			case aiTextureType_EMISSIVE:
				model.m_TextureOptions.push_back(TextureOptions(true));
				break;
			case aiTextureType_GLTF_METALLIC_ROUGHNESS:
				model.m_TextureOptions.push_back(TextureOptions(false));
				break;
			case aiTextureType_NORMALS:
				model.m_TextureOptions.push_back(TextureOptions(false));
				break;
			case aiTextureType_LIGHTMAP:
				model.m_TextureOptions.push_back(TextureOptions(false));
				break;
			default:
				model.m_TextureOptions.push_back(TextureOptions(false));
				break;
			}
		}
		else
		{
			textureData.stringIdx[slot] = 0xFFFF;
		}
	}

	uint32_t CountNodes(const aiNode* node)
	{
		if (!node) return 0;

		uint32_t count = 1;
		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			count += CountNodes(node->mChildren[i]);
		}
		return count;
	}

	bool ModelLoader::LoadModel(ModelData& model, const std::string& filePath)
	{
		uint32_t flag =
			aiProcess_MakeLeftHanded |
			aiProcess_FlipWindingOrder |
			aiProcess_Triangulate |
			aiProcess_CalcTangentSpace |
			aiProcess_GenSmoothNormals |
			aiProcess_LimitBoneWeights |
			aiProcess_SplitLargeMeshes |
			aiProcess_GenUVCoords |
			aiProcess_TransformUVCoords;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filePath, flag);
		if (!scene || !scene->mRootNode)
		{
			return false;
		}

		ProcessMaterial(model, scene);

		model.m_BoundingSphere = BoundingSphere();
		model.m_BoundingBox = AxisAlignedBox();
		uint32_t numNodes = ProcessNode(model, scene, scene->mRootNode, 0, Matrix4x4::IDENTITY);
		model.m_SceneGraph.resize(numNodes);

		ProcessAnimations(model, scene);

		ProcessSkins(model, scene);

		return true;
	}

	uint32_t GetParentIndex(ModelData& model)
	{
		return (uint32_t)model.m_SceneGraph.size() - 1;
	}

	uint32_t ModelLoader::ProcessNode(ModelData& model, const aiScene* scene, const aiNode* node, uint32_t curPos, const Matrix4x4& parentTransform)
	{
		if (!node) return curPos;

		if (model.m_SceneGraph.size() <= curPos)
			model.m_SceneGraph.resize(curPos + 1);

		GraphNode& graphNode = model.m_SceneGraph[curPos];
		graphNode.matrixIndex = curPos;
		graphNode.hasChildren = node->mNumChildren > 0 ? 1 : 0;
		graphNode.hasSibling = (node->mParent && node != node->mParent->mChildren[node->mParent->mNumChildren - 1]) ? 1 : 0;
		graphNode.skeletonRoot = node->mParent == nullptr ? 1 : 0;
		graphNode.transform = ConvertTransform(node->mTransformation);

		const Matrix4x4 LocalXform = parentTransform * graphNode.transform.GetMatrix();

		BoundingSphere sphereOS;
		AxisAlignedBox boxOS;
		CompileMesh(model, scene,LocalXform, sphereOS, boxOS);
		model.m_BoundingSphere = model.m_BoundingSphere.Union(sphereOS);
		model.m_BoundingBox.AddBoundingBox(boxOS);


		uint32_t nextPos = curPos + 1;

		for (uint32_t i = 0; i < node->mNumChildren; ++i)
		{
			nextPos = ProcessNode(model, scene, node->mChildren[i], nextPos, LocalXform);
		}

		return nextPos;

	}

	void ModelLoader::ProcessMaterial(ModelData& model, const aiScene* scene)
	{
		model.m_TextureNames.resize(scene->mNumTextures);
		for (uint32_t i = 0; i < scene->mNumTextures; ++i)
		{
			model.m_TextureNames[i] = scene->mTextures[i]->mFilename.C_Str();
		}

		model.m_MaterialConstants.resize(scene->mNumMaterials);
		model.m_MaterialTextures.resize(scene->mNumMaterials);

		for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
		{
			const aiMaterial* mat = scene->mMaterials[i];
			MaterialConstantData constants{};
			bool alphaTest = mat->GetTextureCount(aiTextureType_OPACITY) > 0;

			aiColor4D color;
			if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, color))
			{
				constants.baseColorFactor[0] = color.r;
				constants.baseColorFactor[1] = color.g;
				constants.baseColorFactor[2] = color.b;
				constants.baseColorFactor[3] = color.a;
			}
			aiColor3D emissive;
			if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive))
			{
				constants.emissiveFactor[0] = emissive.r;
				constants.emissiveFactor[1] = emissive.g;
				constants.emissiveFactor[2] = emissive.b;
			}
			float metallic = 0.0f;
			if (AI_SUCCESS == mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic))
			{
				constants.metallicFactor = metallic;
			}
			float roughness = 0.5f;
			if (AI_SUCCESS == mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness))
			{
				constants.roughnessFactor = roughness;
			}

			constants.normalTextureScale = 1.0f;
			constants.flags = 0x3C000000;
			if (alphaTest)
				constants.flags |= 0x60;
			MaterialTextureData textureData{};

			LoadTexture(model, textureData, mat, aiTextureType_BASE_COLOR, TextureType::kBaseColor, alphaTest);
			LoadTexture(model, textureData, mat, aiTextureType_NORMALS, TextureType::kNormal);
			LoadTexture(model, textureData, mat, aiTextureType_GLTF_METALLIC_ROUGHNESS, TextureType::kMetallicRoughness);
			LoadTexture(model, textureData, mat, aiTextureType_EMISSIVE, TextureType::kEmissive);
			LoadTexture(model, textureData, mat, aiTextureType_LIGHTMAP, TextureType::kOcclusion);

			model.m_MaterialConstants.push_back(constants);
			model.m_MaterialTextures.push_back(textureData);
		}

		ASSERT(model.m_TextureOptions.size() == model.m_TextureNames.size());
	}

	void ModelLoader::ProcessAnimations(ModelData& model, const aiScene* scene)
	{
		model.m_Animations.clear();
		model.m_AnimationCurves.clear();
		model.m_AnimationKeyFrameData.clear();

		for (uint32_t i = 0; i < scene->mNumAnimations; ++i)
		{
			const aiAnimation* aiAnim = scene->mAnimations[i];
			AnimationSet animSet{};
			animSet.duration = static_cast<float>(aiAnim->mDuration);
			animSet.firstCurve = static_cast<uint32_t>(model.m_AnimationCurves.size());
			animSet.numCurves = aiAnim->mNumChannels;

			for (uint32_t j = 0; j < aiAnim->mNumChannels; ++j)
			{
				const aiNodeAnim* channel = aiAnim->mChannels[j];
				AnimationCurve curve{};
				curve.targetNode = FindNodeIndex(channel->mNodeName.C_Str());

				if (channel->mNumPositionKeys > 0)
					curve.targetPath = AnimationCurve::kTranslation;
				else if (channel->mNumRotationKeys > 0)
					curve.targetPath = AnimationCurve::kRotation;
				else if (channel->mNumScalingKeys > 0)
					curve.targetPath = AnimationCurve::kScale;
				else
					continue;

				curve.interpolation = AnimationCurve::kLinear;

				uint32_t numKeys = 0;
				float startTime = 0.0f;
				float endTime = 0.0f;

				if (curve.targetPath == AnimationCurve::kTranslation && channel->mNumPositionKeys > 0)
				{
					numKeys = channel->mNumPositionKeys;
					startTime = static_cast<float>(channel->mPositionKeys[0].mTime);
					endTime = static_cast<float>(channel->mPositionKeys[numKeys - 1].mTime);
				}
				else if (curve.targetPath == AnimationCurve::kRotation && channel->mNumRotationKeys > 0)
				{
					numKeys = channel->mNumRotationKeys;
					startTime = static_cast<float>(channel->mRotationKeys[0].mTime);
					endTime = static_cast<float>(channel->mRotationKeys[numKeys - 1].mTime);
				}
				else if (curve.targetPath == AnimationCurve::kScale && channel->mNumScalingKeys > 0)
				{
					numKeys = channel->mNumScalingKeys;
					startTime = static_cast<float>(channel->mScalingKeys[0].mTime);
					endTime = static_cast<float>(channel->mScalingKeys[numKeys - 1].mTime);
				}

				curve.startTime = startTime;
				curve.numSegments = static_cast<float>(numKeys - 1);
				curve.rangeScale = (endTime > startTime) ? curve.numSegments / (endTime - startTime) : 0.0f;

				curve.keyFrameFormat = AnimationCurve::kFloat;
				curve.keyFrameStride = (curve.targetPath == AnimationCurve::kRotation) ? 4 : 3;
				curve.keyFrameOffset = static_cast<uint32_t>(model.m_AnimationKeyFrameData.size());

				for (uint32_t k = 0; k < numKeys; ++k)
				{
					if (curve.targetPath == AnimationCurve::kTranslation)
					{
						const aiVector3D& v = channel->mPositionKeys[k].mValue;
						AppendVec3(model.m_AnimationKeyFrameData, v);
					}
					else if (curve.targetPath == AnimationCurve::kRotation)
					{
						const aiQuaternion& q = channel->mRotationKeys[k].mValue;
						AppendQuat(model.m_AnimationKeyFrameData, q);
					}
					else if (curve.targetPath == AnimationCurve::kScale)
					{
						const aiVector3D& s = channel->mScalingKeys[k].mValue;
						AppendVec3(model.m_AnimationKeyFrameData, s);
					}
				}

				model.m_AnimationCurves.push_back(curve);
			}

			model.m_Animations.push_back(animSet);
		}
	}

	void ModelLoader::ProcessSkins(ModelData& model, const aiScene* scene)
	{

	}
	void ModelLoader::CompileMesh(
		ModelData& model,
		const aiScene* scene,
		const Matrix4x4& localTransform,
		BoundingSphere& boundingSphere,
		AxisAlignedBox& aabb)
	{
		struct PrimitiveTemp
		{
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			AxisAlignedBox boxOS;
			BoundingSphere sphereOS;
			uint32_t materialIndex;
		};

		std::vector<PrimitiveTemp> primitives;
		primitives.reserve(scene->mNumMeshes);

		size_t totalVertexSize = 0;
		size_t totalIndexSize = 0;

		BoundingSphere sphereOS;
		AxisAlignedBox bboxOS;

		for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
		{
			const aiMesh* srcMesh = scene->mMeshes[i];
			PrimitiveTemp prim;

			prim.vertices.reserve(srcMesh->mNumVertices);
			prim.indices.reserve(srcMesh->mNumFaces * 3);

			for (uint32_t v = 0; v < srcMesh->mNumVertices; ++v)
			{
				Vertex vert{};

				Vector3 pos(srcMesh->mVertices[v].x, srcMesh->mVertices[v].y, srcMesh->mVertices[v].z);
				vert.position = pos;
				if (srcMesh->HasNormals())
					vert.normal = Vector3(srcMesh->mNormals[v].x, srcMesh->mNormals[v].y, srcMesh->mNormals[v].z);
				if (srcMesh->HasTextureCoords(0))
				{
					vert.texcoord.x = srcMesh->mTextureCoords[0][v].x;
					vert.texcoord.y = srcMesh->mTextureCoords[0][v].y;
				}
				if (srcMesh->HasTangentsAndBitangents())
				{
					Vector3 tangent(srcMesh->mTangents[v].x, srcMesh->mTangents[v].y, srcMesh->mTangents[v].z);
					Vector3 bitangent(srcMesh->mBitangents[v].x, srcMesh->mBitangents[v].y, srcMesh->mBitangents[v].z);
					float handedness = (Math::Dot(Math::Cross(vert.normal, tangent), bitangent) < 0.0f) ? -1.0f : 1.0f;
					vert.tangent = Vector4(tangent.x, tangent.y, tangent.z, handedness);
				}

				vert.position = Math::Transfrom(vert.position, localTransform);
				vert.normal = Math::TransformNormal(vert.normal, localTransform);

				prim.vertices.push_back(vert);
			}

			for (uint32_t f = 0; f < srcMesh->mNumFaces; ++f)
			{
				const aiFace& face = srcMesh->mFaces[f];
				if (face.mNumIndices == 3)
				{
					prim.indices.push_back(face.mIndices[0]);
					prim.indices.push_back(face.mIndices[1]);
					prim.indices.push_back(face.mIndices[2]);
				}
			}

			prim.boxOS = aabb;
			for (size_t v = 1; v < prim.vertices.size(); ++v)
			{
				prim.boxOS.AddPoint(prim.vertices[v].position);
			}

			Vector3 center = (prim.boxOS.GetMin() + prim.boxOS.GetMax()) * 0.5f;
			float radius = (prim.boxOS.GetMax() - prim.boxOS.GetMin()).Length() * 0.5f;
			prim.sphereOS = BoundingSphere(center, radius);

			sphereOS = sphereOS.Union(prim.sphereOS);
			bboxOS.AddBoundingBox(prim.boxOS);

			totalVertexSize += prim.vertices.size() * sizeof(Vertex);
			totalIndexSize += prim.indices.size() * sizeof(uint32_t);
			prim.materialIndex = srcMesh->mMaterialIndex;

			primitives.push_back(std::move(prim));
		}

		boundingSphere = sphereOS;
		aabb = bboxOS;
		model.m_BoundingSphere = sphereOS;
		model.m_BoundingBox = bboxOS;

		uint32_t totalBufferSize = (uint32_t)(totalVertexSize + AlignUp((uint32_t)totalIndexSize, 4));
		uint32_t vbStartOffset = (uint32_t)model.m_GeometryData.size();
		model.m_GeometryData.resize(vbStartOffset + totalBufferSize);
		uint8_t* uploadMem = model.m_GeometryData.data() + vbStartOffset;

		uint32_t curVBOffset = 0;
		uint32_t curIBOffset = (uint32_t)totalVertexSize;

		for (auto& prim : primitives)
		{
			size_t numDraws = 1;
			Mesh* mesh = (Mesh*)malloc(sizeof(Mesh) + sizeof(Mesh::Draw) * (numDraws - 1));
			std::unique_ptr<Mesh> meshObj(mesh);

			size_t vbSize = prim.vertices.size() * sizeof(Vertex);
			size_t ibSize = prim.indices.size() * sizeof(uint32_t);

			meshObj->bounds[0] = prim.sphereOS.GetCenter().x;
			meshObj->bounds[1] = prim.sphereOS.GetCenter().y;
			meshObj->bounds[2] = prim.sphereOS.GetCenter().z;
			meshObj->bounds[3] = prim.sphereOS.GetRadius();

			meshObj->vbOffset = vbStartOffset + curVBOffset;
			meshObj->vbSize = (uint32_t)vbSize;
			meshObj->vbStride = sizeof(Vertex);

			meshObj->vbDepthOffset = 0;
			meshObj->vbDepthSize = 0;

			meshObj->ibOffset = vbStartOffset + curIBOffset;
			meshObj->ibSize = (uint32_t)ibSize;
			meshObj->ibFormat = DXGI_FORMAT_R32_UINT;

			meshObj->meshCBV = 0;                     
			meshObj->materialCBV = prim.materialIndex;
			meshObj->srvTable = 0xFFFF;
			meshObj->samplerTable = 0xFFFF;
			meshObj->psoFlags = 0;
			meshObj->pso = 0xFFFF;


			meshObj->numDraws = (uint16_t)numDraws;

			Mesh::Draw& d = meshObj->draw[0];
			d.primCount = (uint32_t)prim.indices.size();
			d.baseVertex = curVBOffset / sizeof(Vertex);
			d.startIndex = curIBOffset / sizeof(uint32_t);


			std::memcpy(uploadMem + curVBOffset, prim.vertices.data(), vbSize);

			std::memcpy(uploadMem + curIBOffset, prim.indices.data(), ibSize);

			curVBOffset += (uint32_t)vbSize;
			curIBOffset += (uint32_t)AlignUp((uint32_t)ibSize, 4);

			model.m_Meshes.push_back(std::move(meshObj));
		}
	}
}
