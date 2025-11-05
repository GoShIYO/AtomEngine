#include "AssetManager.h"
#include "ModelLoader.h"
#include "DirectXTex.h"
#include "Runtime/Function/Render/RenderSystem.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"

#include <map>
#include <mutex>

namespace
{
	std::wstring sRootPath = L"";
	std::map<std::wstring, std::unique_ptr<AtomEngine::ManagedTexture>>sTextureCache;
	std::map<std::wstring, std::shared_ptr<AtomEngine::Model>> sModelDataCache;
	std::unordered_map<uint32_t, uint32_t> gSamplerPermutations;

	std::mutex sTextureMutex;
	std::mutex sModelMutex;
}

namespace AtomEngine
{
	void AssetManager::Initialize(const std::wstring& rootPath)
	{
		sRootPath = rootPath;
	}

	void AssetManager::Shutdown()
	{
		sTextureCache.clear();
		sModelDataCache.clear();
	}

	void AssetManager::DestroyTexture(const std::wstring& key)
	{
		std::lock_guard<std::mutex> Guard(sTextureMutex);

		auto iter = sTextureCache.find(key);
		if (iter != sTextureCache.end())
			sTextureCache.erase(iter);
	}

	void AssetManager::DestroyModel(const std::wstring& key)
	{
		std::lock_guard<std::mutex> Guard(sTextureMutex);
		auto iter = sModelDataCache.find(key);
		if (iter != sModelDataCache.end())
			sModelDataCache.erase(iter);
	}

	ManagedTexture* FindOrLoadTexture(const std::wstring& fileName, eDefaultTexture fallback, bool forceSRGB)
	{
		ManagedTexture* tex = nullptr;

		{
			std::lock_guard<std::mutex> Guard(sTextureMutex);

			std::wstring key = fileName;
			if (forceSRGB)
				key += L"_sRGB";

			// 既存の管理テクスチャを検索する
			auto iter = sTextureCache.find(key);
			if (iter != sTextureCache.end())
			{
				// テクスチャがすでに作成されている場合は、テクスチャにポイントを返す前に、読み込みが完了していることを確認
				tex = iter->second.get();
				tex->WaitForLoad();
				return tex;
			}
			else
			{
				// 見つからない場合は、新しい管理テクスチャを作成し、読み込みを開始します
				tex = new ManagedTexture(key);
				sTextureCache[key].reset(tex);
			}
		}

		std::shared_ptr<std::vector<byte>> ba = ReadFileSync(sRootPath + fileName);
		tex->CreateFromMemory(ba, fallback, forceSRGB);

		// これは初めての要求なので、呼び出し側がファイルを読み取る必要があることを示します。
		return tex;
	}

	TextureRef AssetManager::LoadTextureFile(const std::wstring& filePath, eDefaultTexture fallback, bool forceSRGB)
	{
		return FindOrLoadTexture(filePath, fallback, forceSRGB);
	}

	TextureRef AssetManager::LoadTextureFile(const std::string& filePath, eDefaultTexture fallback, bool forceSRGB)
	{
		return LoadTextureFile(UTF8ToWString(filePath), fallback, forceSRGB);
	}

	TextureRef AssetManager::LoadCovertTexture(const std::wstring& filePath, eDefaultTexture fallback, bool forceSRGB)
	{
		std::wstring originalFile = filePath;
		CompileTextureOnDemand(originalFile, TextureOptions(true));

		std::wstring ddsFile = RemoveExtension(originalFile) + L".dds";
		return FindOrLoadTexture(ddsFile, fallback, forceSRGB);
	}

	TextureRef AssetManager::LoadCovertTexture(const std::string& filePath, eDefaultTexture fallback, bool forceSRGB)
	{
		return LoadCovertTexture(UTF8ToWString(filePath), fallback, forceSRGB);
	}

	static D3D12_CPU_DESCRIPTOR_HANDLE ResolveTextureSRV(
		const Material& material,
		const Model& model,
		TextureSlot slot,
		const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& DefaultTextures)
	{
		const uint32_t mask = material.textureMask;
		//const uint32_t slotBit = (1u << static_cast<uint8_t>(slot));

		if ((mask & slot) != slot)
		{
			switch (slot)
			{
			case TextureSlot::kBaseColor:         return DefaultTextures[0];
			case TextureSlot::kMetallicRoughness: return DefaultTextures[1];
			case TextureSlot::kMetallic:          return DefaultTextures[1];
			case TextureSlot::kRoughness:         return DefaultTextures[2];
			case TextureSlot::kNormal:            return DefaultTextures[(mask & kMetallicRoughness) ? 2 : 3];
			case TextureSlot::kOcclusion:         return DefaultTextures[(mask & kMetallicRoughness) ? 3 : 4];
			case TextureSlot::kEmissive:          return DefaultTextures[(mask & kMetallicRoughness) ? 4 : 5];
			default:                              return GetDefaultTexture(kWhiteOpaque2D);
			}
		}

		for (size_t i = 0; i < material.textures.size(); ++i)
		{
			if (material.textures[i].slot == slot)
			{
				if (i < model.mTextures.size())
					return model.mTextures[i]->GetSRV();
			}
		}

		switch (slot)
		{
		case TextureSlot::kBaseColor:         return DefaultTextures[0];
		case TextureSlot::kMetallicRoughness: return DefaultTextures[1];
		case TextureSlot::kMetallic:          return DefaultTextures[1];
		case TextureSlot::kRoughness:         return DefaultTextures[2];
		case TextureSlot::kNormal:            return DefaultTextures[(mask & kMetallicRoughness) ? 2 : 3];
		case TextureSlot::kOcclusion:         return DefaultTextures[(mask & kMetallicRoughness) ? 3 : 4];
		case TextureSlot::kEmissive:          return DefaultTextures[(mask & kMetallicRoughness) ? 4 : 5];
		default:                              return GetDefaultTexture(kWhiteOpaque2D);
		}
	}

	void LoadMaterialTexture(
		Model& model,
		const ModelData& modelData,
		const std::vector<MaterialTexture>& textures,
		const std::wstring& basePath)
	{
		static_assert((_alignof(MaterialConstants) & 255) == 0, "CBVs need 256 byte alignment");

		const uint32_t numTextures = (uint32_t)textures.size();
		model.mTextures.resize(numTextures);
		for (size_t ti = 0; ti < numTextures; ++ti)
		{
			std::wstring originalFile = basePath + textures[ti].name;
			bool optionalSRGB =
				(textures[ti].slot & (TextureSlot::kBaseColor | TextureSlot::kEmissive)) ? true : false;
			CompileTextureOnDemand(originalFile, TextureOptions(optionalSRGB));

			std::wstring ddsFile = RemoveExtension(originalFile) + L".dds";
			model.mTextures[ti] = AssetManager::LoadTextureFile(ddsFile);
		}

		const uint32_t numMaterials = (uint32_t)modelData.materials.size();
		std::vector<uint32_t> tableOffsets(numMaterials);
		std::vector<bool> hasMRTextures(numMaterials);

		for (size_t matIdx = 0; matIdx < numMaterials; ++matIdx)
		{
			const auto& material = modelData.materials[matIdx];
			uint32_t mask = material.textureMask;
			hasMRTextures[matIdx] = (mask & kMetallicRoughness) == kMetallicRoughness;

			uint32_t numBindTextures = hasMRTextures[matIdx] ? 5 : 6;

			auto& textureHeap = Renderer::GetTextureHeap();

			DescriptorHandle TextureHandles = textureHeap.Alloc(numBindTextures);
			uint32_t SRVDescriptorTable = textureHeap.GetOffsetOfHandle(TextureHandles);

			uint32_t DestCount = numBindTextures;
			std::vector<uint32_t>SourceCounts(DestCount, 1);

			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> DefaultTextures;
			DefaultTextures.resize(numBindTextures);
			if (hasMRTextures[matIdx])
			{
				DefaultTextures =
				{
					GetDefaultTexture(kWhiteOpaque2D),		// BaseColor
					GetDefaultTexture(kWhiteOpaque2D),		// MetallicRoughness
					GetDefaultTexture(kDefaultNormalMap),	// Normal
					GetDefaultTexture(kWhiteOpaque2D),		// AO
					GetDefaultTexture(kBlackTransparent2D),	// Emissive
				};
			}
			else
			{
				DefaultTextures =
				{
					GetDefaultTexture(kWhiteOpaque2D),		// BaseColor
					GetDefaultTexture(kWhiteOpaque2D),		// Metallic
					GetDefaultTexture(kWhiteOpaque2D),		// Roughness
					GetDefaultTexture(kDefaultNormalMap),	// Normal
					GetDefaultTexture(kWhiteOpaque2D),		// AO
					GetDefaultTexture(kBlackTransparent2D),	// Emissive
				};
			}
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> sourceTextures(numBindTextures);

			uint32_t texIndex = 0;
			if (mask & kMetallicRoughness)
			{
				sourceTextures[texIndex++] = ResolveTextureSRV(material, model, TextureSlot::kBaseColor, DefaultTextures);
				sourceTextures[texIndex++] = ResolveTextureSRV(material, model, TextureSlot::kMetallicRoughness, DefaultTextures);
				sourceTextures[texIndex++] = ResolveTextureSRV(material, model, TextureSlot::kNormal, DefaultTextures);
				sourceTextures[texIndex++] = ResolveTextureSRV(material, model, TextureSlot::kOcclusion, DefaultTextures);
				sourceTextures[texIndex++] = ResolveTextureSRV(material, model, TextureSlot::kEmissive, DefaultTextures);
			}
			else
			{
				sourceTextures[texIndex++] = ResolveTextureSRV(material, model, TextureSlot::kBaseColor, DefaultTextures);
				sourceTextures[texIndex++] = ResolveTextureSRV(material, model, TextureSlot::kMetallic, DefaultTextures);
				sourceTextures[texIndex++] = ResolveTextureSRV(material, model, TextureSlot::kRoughness, DefaultTextures);
				sourceTextures[texIndex++] = ResolveTextureSRV(material, model, TextureSlot::kNormal, DefaultTextures);
				sourceTextures[texIndex++] = ResolveTextureSRV(material, model, TextureSlot::kOcclusion, DefaultTextures);
				sourceTextures[texIndex++] = ResolveTextureSRV(material, model, TextureSlot::kEmissive, DefaultTextures);
			}
			DX12Core::gDevice->CopyDescriptors(1, &TextureHandles, &DestCount,
				DestCount, sourceTextures.data(), SourceCounts.data(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			tableOffsets[matIdx] = SRVDescriptorTable;
		}
		for (size_t i = 0; i < model.mMeshData.size(); ++i)
		{
			auto& mesh = model.mMeshData[i];

			if (!mesh.subMeshes.empty())
				mesh.srvTable = tableOffsets[mesh.subMeshes[0].materialIndex];
			for (size_t j = 0; j < mesh.subMeshes.size(); ++j)
			{
				auto& subMesh = mesh.subMeshes[j];
				subMesh.srvTableIndex = tableOffsets[subMesh.materialIndex];

				if (mesh.psoFlags & kHasSkin)
				{
					if (mesh.psoFlags & kAlphaBlend)
					{
						if (hasMRTextures[subMesh.materialIndex])
							mesh.pso = (uint16_t)PSOIndex::kPSO_Transparent_MRWorkFlow_Skin;
						else mesh.pso = (uint16_t)PSOIndex::kPSO_Transparent_Skin;
						
					}
					else
					{
						if (hasMRTextures[subMesh.materialIndex])
							mesh.pso = (uint16_t)PSOIndex::kPSO_MRWorkFlow_Skin;
						else mesh.pso = (uint16_t)PSOIndex::kPSO_Skin;
					}
				}
				else
				{
					if (mesh.psoFlags & kAlphaBlend)
					{
						if (hasMRTextures[subMesh.materialIndex])
							mesh.pso = (uint16_t)PSOIndex::kPSO_Transparent_MRWorkFlow;
						else mesh.pso = (uint16_t)PSOIndex::kPSO_Transparent;
					}
					else
					{
						if (hasMRTextures[subMesh.materialIndex])
							mesh.pso = (uint16_t)PSOIndex::kPSO_MRWorkFlow;
						else mesh.pso = (uint16_t)PSOIndex::kPSO_Default;
					}
				}
			}
		}
	}

	std::shared_ptr<Model> AtomEngine::AssetManager::LoadModel(const std::wstring& filePath)
	{
		return  LoadModel(WStringToUTF8(filePath));
	}

	std::shared_ptr<Model> AssetManager::LoadModel(const std::string& filePath)
	{
		std::lock_guard<std::mutex> guard(sModelMutex);

		std::wstring filePathW = UTF8ToWString(filePath);
		auto it = sModelDataCache.find(filePathW);
		if (it != sModelDataCache.end())
			return it->second;

		std::wstring basePath = GetBasePath(filePathW);
		std::wstring modelName = RemoveBasePath(filePathW);

		auto model = std::make_shared<Model>();

		ModelData modelData;

		if (!ModelLoader::LoadModel(modelData, filePath))
		{
			Log("Failed to load model: %s", filePath.c_str());
			return nullptr;
		}

		model->mBoundingBox = modelData.boundingBox;
		model->mBoundingSphere = modelData.boundingSphere;

		model->mMeshData = std::move(modelData.meshes);
		model->mSceneGraph = std::move(modelData.graphNodes);

		//頂点データをアップロード
		if (!modelData.vertices.empty())
		{
			size_t vertexBufferSize = modelData.vertices.size() * sizeof(Vertex);

			UploadBuffer vertexUpload;
			vertexUpload.Create(L"VertexUpload", vertexBufferSize);
			memcpy(vertexUpload.Map(), modelData.vertices.data(), vertexBufferSize);
			vertexUpload.Unmap();

			model->mVertexBuffer.Create(
				RemoveExtension(modelName) + L"VertexBuffer",
				uint32_t(vertexBufferSize / sizeof(Vertex)),
				sizeof(Vertex),
				vertexUpload
			);
		}
		//インデックスデータをアップロード
		if (!modelData.indices.empty())
		{
			size_t indexBufferSize = modelData.indices.size() * sizeof(uint32_t);

			UploadBuffer indexUpload;
			indexUpload.Create(L"IndexUpload", indexBufferSize);
			memcpy(indexUpload.Map(), modelData.indices.data(), indexBufferSize);
			indexUpload.Unmap();

			model->mIndexBuffer.Create(
				RemoveExtension(modelName) + L"IndexBuffer_",
				static_cast<uint32_t>(modelData.indices.size()),
				sizeof(uint32_t),
				indexUpload
			);
		}

		//マテリアル定数バッファアップロード
		if (modelData.materials.size() > 0)
		{
			size_t materialSize = modelData.materials.size() * sizeof(MaterialConstants);

			UploadBuffer materialConstants;
			materialConstants.Create(L"Material Constant Upload", materialSize);
			MaterialConstants* materialCBV = (MaterialConstants*)materialConstants.Map();
			for (uint32_t i = 0; i < modelData.materials.size(); ++i)
			{
				MaterialConstants materialCB;
				materialCB.baseColorFactor = modelData.materials[i].baseColorFactor;
                materialCB.emissiveFactor = modelData.materials[i].emissiveFactor;
                materialCB.metallicFactor = modelData.materials[i].metallicFactor;
				materialCB.roughnessFactor = modelData.materials[i].roughnessFactor;

				memcpy(materialCBV, &materialCB, sizeof(MaterialConstants));
				materialCBV++;
			}
			materialConstants.Unmap();
			model->mMaterialConstants.Create(L"Material Constants",
				(uint32_t)modelData.materials.size(), sizeof(MaterialConstants), materialConstants);
		}

		std::vector<MaterialTexture> textures;
		for (uint32_t m = 0; m < modelData.materials.size(); ++m)
		{
			for (uint32_t t = 0; t < modelData.materials[m].textures.size(); ++t)
			{
				textures.push_back(modelData.materials[m].textures[t]);
			}
		}

		LoadMaterialTexture(*model, modelData, textures, basePath);

		if (!modelData.animations.empty())
		{
			model->mAnimationData = std::move(modelData.animations);
		}

		if (!modelData.skeleton.joints.empty())
		{
			
			/*model->mJointIBMs.resize(modelData.skeleton.joints.size());
			for (size_t i = 0; i < modelData.skeleton.joints.size(); ++i)
			{
				model->mJointIBMs[i] = modelData.skeleton.joints[i].inverseBindPose;
			}

			for (auto& [name, cluster] : modelData.skinClusterData)
			{
				for (auto& [vtxIdx, weightData] : cluster.vertexWeights)
				{
					model->mJointIndices.push_back(static_cast<uint16_t>(vtxIdx));
				}
			}*/
			model->mJointIBMs = std::move(modelData.jointIBMs);
            model->mJointIndices = std::move(modelData.jointIndices);
		}

		sModelDataCache[modelName] = model;
		return model;
	}
}
