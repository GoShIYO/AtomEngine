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

	void AssetManager::DestroyModel(const std::wstring& key)
	{
		std::lock_guard<std::mutex> Guard(sMutex);
		auto iter = sModelDataCache.find(key);
		if (iter != sModelDataCache.end())
			sModelDataCache.erase(iter);
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


		sModelDataCache[UTF8ToWString(filePath)] = model;
		return model;
	}
}