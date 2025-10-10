#include "ModelLoader.h"
#include <map>

namespace AtomEngine
{

	std::unordered_map<std::string, uint32_t> nodeNameToIndex;

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


	uint32_t FindNodeIndex(const std::unordered_map<std::string, uint32_t>& nameMap, const std::string& name)
	{
		auto it = nameMap.find(name);
		if (it != nameMap.end())
			return it->second;
		return 0xFFFFFFFF;
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


	void LoadTexture(ModelData& model, MaterialTextureData& textureData, aiMaterial* mat, aiTextureType type, TextureType slot)
	{

		aiString path;
		if (mat->GetTexture(type, 0, &path) == AI_SUCCESS)
		{
			std::string texName = path.C_Str();
			model.m_TextureNames.push_back(texName);
			textureData.stringIdx[slot] = (uint16_t)(model.m_TextureNames.size() - 1);
			switch (type)
			{
			case aiTextureType_DIFFUSE:
				model.m_TextureOptions.push_back(TextureOptions(true));
				textureData.addressModes |= (D3D12_TEXTURE_ADDRESS_MODE_CLAMP << (slot *2));
				break;
			case aiTextureType_EMISSIVE:
				model.m_TextureOptions.push_back(TextureOptions(true));
				textureData.addressModes |= (D3D12_TEXTURE_ADDRESS_MODE_CLAMP << (slot * 2));
				break;
			case aiTextureType_METALNESS:
				model.m_TextureOptions.push_back(TextureOptions(false));
				textureData.addressModes |= (D3D12_TEXTURE_ADDRESS_MODE_CLAMP << (slot * 2));
				break;
			case aiTextureType_DIFFUSE_ROUGHNESS:
				model.m_TextureOptions.push_back(TextureOptions(false));
				textureData.addressModes |= (D3D12_TEXTURE_ADDRESS_MODE_CLAMP << (slot * 2));
				break;
			case aiTextureType_NORMALS:
				model.m_TextureOptions.push_back(TextureOptions(true));
				textureData.addressModes |= (D3D12_TEXTURE_ADDRESS_MODE_WRAP << (slot * 2));
				break;
			}
		}
		else
		{
			textureData.stringIdx[slot] = 0xFFFF;
		}
	}

	bool ModelLoader::LoadModel(ModelData& model, const std::string& filePath)
	{
		uint32_t flag = 
			aiProcess_MakeLeftHanded |
			aiProcess_FlipWindingOrder |
			aiProcess_Triangulate |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_OptimizeMeshes |
			aiProcess_OptimizeGraph |
			aiProcess_CalcTangentSpace |
			aiProcess_GenSmoothNormals |
			aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality |
			aiProcess_LimitBoneWeights |
			aiProcess_SplitLargeMeshes |
			aiProcess_GenUVCoords |
			aiProcess_TransformUVCoords |
			aiProcess_FindInvalidData;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filePath, flag);
		assert(scene && scene->mRootNode);

		for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
		{
			ProcessMaterial(model, scene->mMaterials[i]);
		}

		model.m_Meshes.reserve(scene->mNumMeshes);
		for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
		{
			auto mesh = ProcessMesh(model, scene->mMeshes[i], scene);
			if (mesh)
			{
				model.m_Meshes.push_back(std::move(mesh));
			}
		}

		ProcessNode(model, scene->mRootNode, scene, Matrix4x4());

		if (scene->HasAnimations())
		{
			ProcessAnimations(model, scene);
		}

		BuildGeometryData(model, scene);

		ComputeBoundingVolumes(model,scene);

		return true;
	}

	uint32_t GetParentIndex(ModelData& model)
	{
		return (uint32_t)model.m_SceneGraph.size() - 1;
	}

	void ModelLoader::ProcessNode(ModelData& model, aiNode* node, const aiScene* scene, const Matrix4x4& parentTransform)
	{
		uint32_t nodeIndex = static_cast<uint32_t>(model.m_SceneGraph.size());

		GraphNode graphNode{};
		graphNode.transform = ConvertTransform(node->mTransformation);
		graphNode.matrixIndex = nodeIndex;
		graphNode.hasChildren = (node->mNumChildren > 0) ? 1u : 0u;
		graphNode.staleMatrix = true;
		graphNode.skeletonRoot = false;
		graphNode.parentIndex = (node->mParent) ? FindNodeIndex(nodeNameToIndex, node->mParent->mName.C_Str()) : 0xFFFFFFFFu;

		if (node->mNumMeshes > 0)
		{
			graphNode.meshStart = static_cast<uint32_t>(model.m_Meshes.size());

			graphNode.meshStart = static_cast<uint32_t>(node->mMeshes[0]);
			graphNode.meshCount = node->mNumMeshes;
		}
		else
		{
			graphNode.meshStart = 0xFFFFFFFFu;
			graphNode.meshCount = 0;
		}

		model.m_SceneGraph.push_back(graphNode);
		nodeNameToIndex[node->mName.C_Str()] = nodeIndex;

		// children
		for (uint32_t i = 0; i < node->mNumChildren; ++i)
			ProcessNode(model, node->mChildren[i], scene, parentTransform);
	}


	std::unique_ptr<Mesh> ModelLoader::ProcessMesh(ModelData& model, aiMesh* srcMesh, const aiScene* scene)
	{
		auto mesh = std::make_unique<Mesh>();
		mesh->materialCBV = srcMesh->mMaterialIndex;

		mesh->psoFlags = PSOFlags::kHasPosition | PSOFlags::kHasNormal;

		// TANGENT
		if (srcMesh->HasTangentsAndBitangents())
		{
			mesh->psoFlags |= PSOFlags::kHasTangent;
		}

		// UV0
		if (srcMesh->HasTextureCoords(0))
		{
			mesh->psoFlags |= PSOFlags::kHasUV0;
		}

		// UV1
		if (srcMesh->HasTextureCoords(1))
		{
			mesh->psoFlags |= PSOFlags::kHasUV1;
		}

		// Skinning (BLENDINDICES/BLENDWEIGHT)
		if (srcMesh->HasBones())
		{
			mesh->numJoints = srcMesh->mNumBones;
			mesh->startJoint = static_cast<uint16_t>(model.m_JointIndices.size());
			mesh->startJoint = static_cast<uint16_t>(model.m_JointIndices.size());

			for (uint32_t i = 0; i < srcMesh->mNumBones; ++i)
			{
				const aiBone* bone = srcMesh->mBones[i];
				std::string boneName = bone->mName.C_Str();

				uint32_t nodeIndex = FindNodeIndex(nodeNameToIndex, boneName);
	
				Matrix4x4 inverseBindMatrix = Matrix4x4((float*)&bone->mOffsetMatrix);
				model.m_JointIBMs.push_back(inverseBindMatrix);

				model.m_JointIndices.push_back(static_cast<uint16_t>(nodeIndex));
			}
			mesh->psoFlags |= PSOFlags::kHasSkin;
		}

		mesh->vbStride = sizeof(float) * (4 + 2 + 3 + 3 + 3);
		mesh->ibFormat = DXGI_FORMAT_R16_UINT;

		DrawCall drawCall;
		drawCall.primCount = srcMesh->mNumFaces * 3;
		drawCall.startIndex = 0;
		drawCall.baseVertex = 0;
		mesh->draw.push_back(drawCall);
		mesh->numDraws = 1;

		return mesh;
	}

	void ModelLoader::ProcessMaterial(ModelData& model, aiMaterial* mat)
	{
		MaterialConstantData constants{};
		constants.baseColorFactor[0] = 1.0f;
		constants.baseColorFactor[1] = 1.0f;
		constants.baseColorFactor[2] = 1.0f;
		constants.baseColorFactor[3] = 1.0f;
		constants.metallicFactor = 1.0f;
		constants.roughnessFactor = 1.0f;
		constants.normalTextureScale = 1.0f;

		MaterialTextureData textureData{};

		LoadTexture(model, textureData, mat, aiTextureType_BASE_COLOR, TextureType::kBaseColor);
		LoadTexture(model, textureData, mat, aiTextureType_NORMALS, TextureType::kNormal);
		LoadTexture(model, textureData, mat, aiTextureType_METALNESS, TextureType::kMetallic);
		LoadTexture(model, textureData, mat, aiTextureType_DIFFUSE_ROUGHNESS, TextureType::kRoughness);
		LoadTexture(model, textureData, mat, aiTextureType_EMISSIVE, TextureType::kEmissive);
		LoadTexture(model, textureData, mat, aiTextureType_AMBIENT_OCCLUSION, TextureType::kOcclusion);

		model.m_MaterialConstants.push_back(constants);
		model.m_MaterialTextures.push_back(textureData);
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

				curve.targetNode = FindNodeIndex(nodeNameToIndex, channel->mNodeName.C_Str());

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

	void ModelLoader::ComputeBoundingVolumes(ModelData& model, const aiScene* scene)
	{
		AxisAlignedBox totalAABB;

		for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
		{
			const aiMesh* srcMesh = scene->mMeshes[i];

			for (unsigned int v = 0; v < srcMesh->mNumVertices; ++v)
			{
				const aiVector3D& pos = srcMesh->mVertices[v];

				totalAABB.AddPoint(Vector3(pos.x, pos.y, pos.z));
			}
		}

		model.m_BoundingBox = totalAABB;

		Vector3 center = totalAABB.GetCenter();

		Vector3 dimensions = totalAABB.GetDimensions();

		float radius = dimensions.Length() * 0.5f;

		model.m_BoundingSphere = BoundingSphere(center, radius);
	}

	void ModelLoader::BuildGeometryData(ModelData& model, const aiScene* scene)
	{
		// CPUデータバッファを構築
		std::vector<uint8_t> totalVertexData;
		std::vector<uint16_t> totalIndexData;

		uint32_t currentVertexByteOffset = 0;
		uint32_t totalIndexCount = 0;
		uint32_t maxVertexStride = 0;

		// 各メッシュのデータを収集し、オフセットを計算
		for (size_t i = 0; i < model.m_Meshes.size(); ++i)
		{
			aiMesh* srcMesh = scene->mMeshes[i];
			auto& mesh = model.m_Meshes[i];

			std::vector<uint8_t> meshVertexData;
			std::vector<uint16_t> meshIndexData;
			uint32_t currentStride = 0;

			// 頂点データとストライドを生成
			CreateVertexData(srcMesh, meshVertexData, currentStride);

			// インデックスデータを生成
			CreateIndexData(srcMesh, meshIndexData);

			totalVertexData.insert(totalVertexData.end(), meshVertexData.begin(), meshVertexData.end());
			totalIndexData.insert(totalIndexData.end(), meshIndexData.begin(), meshIndexData.end());

			if (mesh->vbStride != currentStride)
			{
				mesh->vbStride = (uint8_t)currentStride;
			}
			maxVertexStride = std::max(maxVertexStride, currentStride);

			// サイズを正確に設定
			mesh->vbSize = (uint32_t)meshVertexData.size();
			mesh->ibSize = (uint32_t)meshIndexData.size() * sizeof(uint16_t);

			// オフセットを設定
			mesh->vbOffset = currentVertexByteOffset;
			// 深度用もフルサイズと仮定
			mesh->vbDepthOffset = mesh->vbOffset;
			mesh->vbDepthSize = mesh->vbSize;

			// インデックスバッファのオフセットは、全頂点データの総バイトサイズからの相対位置
			mesh->ibOffset = (uint32_t)totalVertexData.size() - mesh->ibSize;

			// 全体サイズを更新
			currentVertexByteOffset += mesh->vbSize;
			totalIndexCount += (uint32_t)meshIndexData.size();

			// DrawCallのプリミティブ数も再確認
			mesh->draw[0].primCount = (uint32_t)meshIndexData.size();
		}

		uint32_t totalVertexByteSize = (uint32_t)totalVertexData.size();
		uint32_t totalIndexByteSize = (uint32_t)totalIndexData.size() * sizeof(uint16_t);

		model.m_GeometryData.resize(totalVertexByteSize + totalIndexByteSize);

		// データコピー
		if (totalVertexByteSize > 0)
		{
			std::memcpy(model.m_GeometryData.data(), totalVertexData.data(), totalVertexByteSize);
		}
		if (totalIndexByteSize > 0)
		{
			std::memcpy(model.m_GeometryData.data() + totalVertexByteSize, totalIndexData.data(), totalIndexByteSize);
		}

		// ビューの構築
		model.m_VertexBuffer.BufferLocation = 0;
		model.m_VertexBuffer.SizeInBytes = totalVertexByteSize;
		model.m_VertexBuffer.StrideInBytes = maxVertexStride;

		model.m_IndexBuffer.BufferLocation = totalVertexByteSize;
		model.m_IndexBuffer.SizeInBytes = totalIndexByteSize;
		model.m_IndexBuffer.Format = DXGI_FORMAT_R16_UINT;

		// 深度バッファビュー
		model.m_VertexBufferDepth = model.m_VertexBuffer;
		model.m_IndexBufferDepth = model.m_IndexBuffer;
	}

	void ModelLoader::CreateVertexData(aiMesh* srcMesh, std::vector<uint8_t>& vertexData, uint32_t& vertexStride)
	{
		const bool hasTangents = srcMesh->HasTangentsAndBitangents();
		const bool hasUV0 = srcMesh->HasTextureCoords(0);
		const bool hasBones = srcMesh->HasBones();

		// stride (floats + uint16)
		uint32_t floatCount = 4 + 2 + 3; // pos(4) + uv(2) + normal(3)
		if (hasTangents) floatCount += 3 + 3; // tangent + bitangent
		uint32_t floatBytes = floatCount * sizeof(float);
		uint32_t extraBytes = hasBones ? (sizeof(uint16_t) * 8) : 0; // 4 indices + 4 weights
		vertexStride = floatBytes + extraBytes;

		vertexData.reserve(srcMesh->mNumVertices * vertexStride);

		std::unordered_map<uint32_t, std::vector<std::pair<uint16_t, float>>> vertexBoneMap;
		if (hasBones)
		{
			for (uint32_t bi = 0; bi < srcMesh->mNumBones; ++bi)
			{
				const aiBone* bone = srcMesh->mBones[bi];
				uint16_t localIndex = static_cast<uint16_t>(bi);
				for (uint32_t w = 0; w < bone->mNumWeights; ++w)
				{
					const aiVertexWeight& vw = bone->mWeights[w];
					vertexBoneMap[vw.mVertexId].push_back({ localIndex, vw.mWeight });
				}
			}
		}

		for (unsigned int i = 0; i < srcMesh->mNumVertices; ++i)
		{
			float px = srcMesh->mVertices[i].x;
			float py = srcMesh->mVertices[i].y;
			float pz = srcMesh->mVertices[i].z;
			float pw = 1.0f;
			vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&px), reinterpret_cast<uint8_t*>(&px) + sizeof(float));
			vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&py), reinterpret_cast<uint8_t*>(&py) + sizeof(float));
			vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&pz), reinterpret_cast<uint8_t*>(&pz) + sizeof(float));
			vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&pw), reinterpret_cast<uint8_t*>(&pw) + sizeof(float));

			float uvx = 0.0f, uvy = 0.0f;
			if (hasUV0)
			{
				uvx = srcMesh->mTextureCoords[0][i].x;
				uvy = srcMesh->mTextureCoords[0][i].y;
			}
			vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&uvx), reinterpret_cast<uint8_t*>(&uvx) + sizeof(float));
			vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&uvy), reinterpret_cast<uint8_t*>(&uvy) + sizeof(float));

			if (srcMesh->HasNormals())
			{
				float nx = srcMesh->mNormals[i].x;
				float ny = srcMesh->mNormals[i].y;
				float nz = srcMesh->mNormals[i].z;
				vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&nx), reinterpret_cast<uint8_t*>(&nx) + sizeof(float));
				vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&ny), reinterpret_cast<uint8_t*>(&ny) + sizeof(float));
				vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&nz), reinterpret_cast<uint8_t*>(&nz) + sizeof(float));
			}
			else
			{
				float z0 = 0.0f;
				vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&z0), reinterpret_cast<uint8_t*>(&z0) + sizeof(float) * 3);
			}

			if (hasTangents)
			{
				float tx = srcMesh->mTangents[i].x;
				float ty = srcMesh->mTangents[i].y;
				float tz = srcMesh->mTangents[i].z;
				vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&tx), reinterpret_cast<uint8_t*>(&tx) + sizeof(float));
				vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&ty), reinterpret_cast<uint8_t*>(&ty) + sizeof(float));
				vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&tz), reinterpret_cast<uint8_t*>(&tz) + sizeof(float));

				float bx = srcMesh->mBitangents[i].x;
				float by = srcMesh->mBitangents[i].y;
				float bz = srcMesh->mBitangents[i].z;
				vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&bx), reinterpret_cast<uint8_t*>(&bx) + sizeof(float));
				vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&by), reinterpret_cast<uint8_t*>(&by) + sizeof(float));
				vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(&bz), reinterpret_cast<uint8_t*>(&bz) + sizeof(float));
			}

			if (hasBones)
			{
				uint16_t indices[4] = { 0,0,0,0 };
				uint16_t weights[4] = { 0,0,0,0 };

				auto it = vertexBoneMap.find(i);
				if (it != vertexBoneMap.end())
				{
					auto& vec = it->second;
					std::sort(vec.begin(), vec.end(), [](auto& a, auto& b) { return a.second > b.second; });
					float total = 0.0f;
					size_t k = 0;
					for (; k < vec.size() && k < 4; ++k)
					{
						indices[k] = vec[k].first;
						total += vec[k].second;
					}

					for (size_t j = 0; j < k; ++j)
					{
						float norm = (total > 0.0f) ? (vec[j].second / total) : 0.0f;
						weights[j] = static_cast<uint16_t>(std::round(norm * 65535.0f));
					}
				}
				vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(indices), reinterpret_cast<uint8_t*>(indices) + sizeof(indices));
				vertexData.insert(vertexData.end(), reinterpret_cast<uint8_t*>(weights), reinterpret_cast<uint8_t*>(weights) + sizeof(weights));
			}
		}
	}


	void ModelLoader::CreateIndexData(aiMesh* srcMesh, std::vector<uint16_t>& indexData)
	{
		indexData.reserve(srcMesh->mNumFaces * 3);

		for (unsigned int i = 0; i < srcMesh->mNumFaces; ++i)
		{
			const aiFace& face = srcMesh->mFaces[i];
			assert(face.mNumIndices == 3);
			indexData.push_back(static_cast<uint16_t>(face.mIndices[0]));
			indexData.push_back(static_cast<uint16_t>(face.mIndices[1]));
			indexData.push_back(static_cast<uint16_t>(face.mIndices[2]));
		}
	}

}
