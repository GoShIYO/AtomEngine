#include "AssetManager.h"
#include "ModelLoader.h"
#include "DirectXTex.h"
#include "Runtime/Function/Render/RenderSystem.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"

#include <map>
#include <mutex>

namespace
{
	std::wstring sRootPath = L"";
	std::map<std::wstring, std::unique_ptr<AtomEngine::ManagedTexture>>sTextureCache;
	std::map<std::wstring, std::shared_ptr<AtomEngine::Model>> sModelDataCache;
	std::unordered_map<uint32_t, uint32_t> gSamplerPermutations;

	std::mutex sMutex;
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
	}

	void AssetManager::DestroyTexture(const std::wstring& key)
	{
		std::lock_guard<std::mutex> Guard(sMutex);

		auto iter = sTextureCache.find(key);
		if (iter != sTextureCache.end())
			sTextureCache.erase(iter);
	}

	ManagedTexture* FindOrLoadTexture(const std::wstring& fileName, eDefaultTexture fallback, bool forceSRGB)
	{
		ManagedTexture* tex = nullptr;

		{
			std::lock_guard<std::mutex> Guard(sMutex);

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

	D3D12_CPU_DESCRIPTOR_HANDLE GetSampler(uint32_t addressModes)
	{
		SamplerDesc samplerDesc;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE(addressModes & 0x3);
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE(addressModes >> 2);
		return samplerDesc.CreateDescriptor();
	}

	void LoadMaterials(Model& model,
		const std::vector<MaterialTextureData>& materialTextures,
		const std::vector<std::wstring>& textureNames,
		const std::vector<uint8_t>& textureOptions,
		const std::wstring& basePath)
	{
		// Load textures
		const uint32_t numTextures = (uint32_t)textureNames.size();
		model.textures.resize(numTextures);
		for (size_t ti = 0; ti < numTextures; ++ti)
		{
			std::wstring originalFile = basePath + textureNames[ti];
			CompileTextureOnDemand(originalFile, textureOptions[ti]);

			std::wstring ddsFile = RemoveExtension(originalFile) + L".dds";
			model.textures[ti] = AssetManager::LoadTextureFile(ddsFile);
		}

		// 記述子テーブルを生成し、各マテリアルのオフセットを記録する
		const uint32_t numMaterials = (uint32_t)materialTextures.size();
		std::vector<uint32_t> tableOffsets(numMaterials);

		auto textureHeap = RenderSystem::GetTextureHeap();
		auto samplerHeap = RenderSystem::GetSamplerHeap();

		for (uint32_t matIdx = 0; matIdx < numMaterials; ++matIdx)
		{
			const MaterialTextureData& srcMat = materialTextures[matIdx];

			DescriptorHandle TextureHandles = textureHeap.Alloc(kNumTextures);
			uint32_t SRVDescriptorTable = textureHeap.GetOffsetOfHandle(TextureHandles);

			uint32_t DestCount = kNumTextures;
			uint32_t SourceCounts[kNumTextures] = { 1, 1, 1, 1, 1 };

			D3D12_CPU_DESCRIPTOR_HANDLE DefaultTextures[kNumTextures] =
			{
				GetDefaultTexture(kWhiteOpaque2D),			//baseColor
				GetDefaultTexture(kWhiteOpaque2D),			//matallicRoughness
				GetDefaultTexture(kWhiteOpaque2D),			//occlusion
				GetDefaultTexture(kBlackTransparent2D),		//emissive
				GetDefaultTexture(kDefaultNormalMap)		//kNormalMap
			};

			D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[kNumTextures];
			for (uint32_t j = 0; j < kNumTextures; ++j)
			{
				if (srcMat.stringIdx[j] == 0xffff)
					SourceTextures[j] = DefaultTextures[j];
				else
					SourceTextures[j] = model.textures[srcMat.stringIdx[j]].GetSRV();
			}

			DX12Core::gDevice->CopyDescriptors(1, &TextureHandles, &DestCount,
				DestCount, SourceTextures, SourceCounts, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			// このサンプラーの組み合わせが以前に使用されたことがあるかどうかを確認します
			// 。使用されていない場合は、ヒープからさらに割り当て、記述子をコピーします。
			uint32_t addressModes = srcMat.addressModes;
			auto samplerMapLookup = gSamplerPermutations.find(addressModes);

			if (samplerMapLookup == gSamplerPermutations.end())
			{
				DescriptorHandle SamplerHandles = samplerHeap.Alloc(kNumTextures);
				uint32_t SamplerDescriptorTable = samplerHeap.GetOffsetOfHandle(SamplerHandles);

				gSamplerPermutations[addressModes] = SamplerDescriptorTable;
				tableOffsets[matIdx] = SRVDescriptorTable | SamplerDescriptorTable << 16;

				D3D12_CPU_DESCRIPTOR_HANDLE SourceSamplers[kNumTextures];
				for (uint32_t j = 0; j < kNumTextures; ++j)
				{
					SourceSamplers[j] = GetSampler(addressModes & 0xF);
					addressModes >>= 4;
				}
				DX12Core::gDevice->CopyDescriptors(1, &SamplerHandles, &DestCount,
					DestCount, SourceSamplers, SourceCounts, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
			}
			else
			{
				tableOffsets[matIdx] = SRVDescriptorTable | samplerMapLookup->second << 16;
			}
		}

		//各メッシュのテーブルオフセットを更新します
		uint8_t* meshPtr = model.mMeshData.get();
		for (uint32_t i = 0; i < model.mNumMeshes; ++i)
		{
			Mesh& mesh = *(Mesh*)meshPtr;
			uint32_t offsetPair = tableOffsets[mesh.materialCBV];
			mesh.srvTable = offsetPair & 0xFFFF;
			mesh.samplerTable = offsetPair >> 16;
			mesh.pso = RenderSystem::GetPSO(mesh.psoFlags);
			meshPtr += sizeof(Mesh) + (mesh.numDraws - 1) * sizeof(Mesh::Draw);
		}
	}

	std::shared_ptr<Model> AtomEngine::AssetManager::LoadModel(const std::wstring& filePath)
	{
		return  LoadModel(WStringToUTF8(filePath));
	}

	std::shared_ptr<Model> AssetManager::LoadModel(const std::string& filePath)
	{
		//std::lock_guard<std::mutex> guard(sMutex);

		auto it = sModelDataCache.find(UTF8ToWString(filePath));
		if (it != sModelDataCache.end())
			return it->second;

		std::string basePath = GetBasePath(filePath);

		auto model = std::make_shared<Model>();

		ModelData modelData;
		ModelLoader::LoadModel(modelData, filePath);

		model->mNumNodes = (uint32_t)modelData.m_SceneGraph.size();
		model->mSceneGraph.reset(new GraphNode[modelData.m_SceneGraph.size()]);

		uint32_t meshDataSize = 0;
		for (const auto& mesh : modelData.m_Meshes)
			meshDataSize += (uint32_t)sizeof(Mesh) + (mesh->numDraws - 1) * (uint32_t)sizeof(Mesh::Draw);
		model->mNumMeshes = (uint32_t)modelData.m_Meshes.size();
		model->mMeshData.reset(new uint8_t[meshDataSize]);

		uint8_t* meshDst = model->mMeshData.get();
		for (const auto& mesh : modelData.m_Meshes)
		{
			size_t meshSize = sizeof(Mesh) + (mesh->numDraws - 1) * sizeof(Mesh::Draw);
			memcpy(meshDst, mesh.get(), meshSize);
			meshDst += meshSize;
		}

		if (modelData.m_GeometryData.size() > 0)
		{
			UploadBuffer modelUploadBuffer;
			modelUploadBuffer.Create(L"Model Data Upload", modelData.m_GeometryData.size());
			memcpy(modelUploadBuffer.Map(), modelData.m_GeometryData.data(), modelData.m_GeometryData.size());
			modelUploadBuffer.Unmap();
			model->mDataBuffer.Create(L"Model Data", (uint32_t)modelData.m_GeometryData.size(), 1, modelUploadBuffer);
		}

		memcpy(model->mSceneGraph.get(), modelData.m_SceneGraph.data(), modelData.m_SceneGraph.size() * sizeof(GraphNode));


		auto numMaterials = modelData.m_MaterialConstants.size();
		auto numTextures = modelData.m_TextureNames.size();

		if (numMaterials > 0)
		{
			UploadBuffer materialConstants;
			materialConstants.Create(L"Material Constants Upload", numMaterials * sizeof(MaterialConstants));
			auto materialCBV = (MaterialConstants*)materialConstants.Map();
			for (uint32_t i = 0; i < numMaterials; ++i)
			{
				MaterialConstants materialCB;

				const MaterialConstantData& src = modelData.m_MaterialConstants[i];
				memcpy(materialCB.baseColorFactor, src.baseColorFactor, 4 * sizeof(float));
				memcpy(materialCB.emissiveFactor, src.emissiveFactor, 3 * sizeof(float));
				materialCB.emissiveFactor[3] = 1.0f;
				materialCB.metallicFactor = src.metallicFactor;
				materialCB.roughnessFactor = src.roughnessFactor;
				materialCB.flags = src.flags;

				memcpy(&materialCBV[i], &materialCB, sizeof(MaterialConstants));
			}
			materialConstants.Unmap();
			model->mMaterialConstants.Create(L"Material Constants", (uint32_t)numMaterials, sizeof(MaterialConstants), materialConstants);
		}

		std::vector<MaterialTextureData> materialTextures(numMaterials);
		for (uint32_t i = 0; i < numMaterials; ++i)
		{
			materialTextures[i] = modelData.m_MaterialTextures[i];
		}
		std::vector<std::wstring> textureNames(numTextures);
		for (uint32_t i = 0; i < numTextures; ++i)
		{
			textureNames[i] = UTF8ToWString(modelData.m_TextureNames[i]);
		}
		std::vector<uint8_t> textureOptions(numTextures);
		for (uint32_t i = 0; i < numTextures; ++i)
		{
			textureOptions[i] = modelData.m_TextureOptions[i];
		}

		LoadMaterials(*model, materialTextures, textureNames, textureOptions, UTF8ToWString(basePath));

		model->m_BoundingSphere = modelData.m_BoundingSphere;
		model->m_BoundingBox = modelData.m_BoundingBox;
		model->mNumAnimations = (uint32_t)modelData.m_Animations.size();

		if (modelData.m_Animations.size() > 0)
		{

			ASSERT(modelData.m_AnimationKeyFrameData.size() > 0 && modelData.m_AnimationCurves.size() > 0);
			model->mKeyFrameData.reset(new uint8_t[modelData.m_AnimationKeyFrameData.size()]);
			memcpy(model->mKeyFrameData.get(), modelData.m_AnimationKeyFrameData.data(), modelData.m_AnimationKeyFrameData.size() * sizeof(uint8_t));

			model->mCurveData.reset(new AnimationCurve[modelData.m_AnimationCurves.size()]);
			memcpy(model->mCurveData.get(), modelData.m_AnimationCurves.data(), modelData.m_AnimationCurves.size() * sizeof(AnimationCurve));

			model->mAnimations.reset(new AnimationSet[modelData.m_Animations.size()]);
			memcpy(model->mAnimations.get(), modelData.m_Animations.data(), modelData.m_Animations.size() * sizeof(AnimationSet));

		}

		model->mNumJoints = (uint32_t)modelData.m_JointIndices.size();

		if (modelData.m_JointIndices.size() > 0)
		{
			model->mJointIndices.reset(new uint16_t[modelData.m_JointIndices.size()]);
			for (uint32_t i = 0; i < modelData.m_JointIndices.size(); ++i)
				model->mJointIndices[i] = modelData.m_JointIndices[i];
			model->mJointIBMs.reset(new Matrix4x4[modelData.m_JointIBMs.size()]);
			for (uint32_t i = 0; i < modelData.m_JointIBMs.size(); ++i)
				model->mJointIBMs[i] = modelData.m_JointIBMs[i];
		}

		sModelDataCache[UTF8ToWString(filePath)] = model;
		return model;
	}
}