#include "AssetManager.h"
#include "DirectXTex.h"
#include <map>
#include <mutex>

namespace
{
	std::wstring sRootPath = L"";
	std::map<std::wstring, std::unique_ptr<AtomEngine::ManagedTexture>>sTextureCache;
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
		std::wstring originalFile = filePath;
		CompileTextureOnDemand(originalFile, TextureOptions(true));

		std::wstring ddsFile = RemoveExtension(originalFile) + L".dds";
		return FindOrLoadTexture(ddsFile, fallback, forceSRGB);
	}

	TextureRef AssetManager::LoadTextureFile(const std::string& filePath, eDefaultTexture fallback, bool forceSRGB)
	{
		return LoadTextureFile(UTF8ToWString(filePath), fallback, forceSRGB);
	}
}