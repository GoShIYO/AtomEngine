#pragma once
#include "TextureRef.h"
#include <string>

namespace AtomEngine
{
	class Model;
	class AssetManager
	{
	public:
		static void Initialize(const std::wstring& RootPath);

		static void Shutdown();

		static void DestroyTexture(const std::wstring& key);

		static TextureRef LoadTextureFile(const std::wstring& filePath, eDefaultTexture fallback = kMagenta2D, bool sRGB = false);
		static TextureRef LoadTextureFile(const std::string& filePath, eDefaultTexture fallback = kMagenta2D, bool sRGB = false);

		static TextureRef LoadCovertTexture(const std::wstring& filePath, eDefaultTexture fallback = kMagenta2D, bool sRGB = false);
        static TextureRef LoadCovertTexture(const std::string& filePath, eDefaultTexture fallback = kMagenta2D, bool sRGB = false);

		static std::shared_ptr<Model> LoadModel(const std::wstring& filePath);
		static std::shared_ptr<Model> LoadModel(const std::string& filePath);

	};
}


