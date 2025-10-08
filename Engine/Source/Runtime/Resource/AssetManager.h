#pragma once
#include "TextureRef.h"
#include <string>

namespace AtomEngine
{
	class AssetManager
	{
	public:
		static void Initialize(const std::wstring& RootPath);

		static void Shutdown();

		static void DestroyTexture(const std::wstring& key);

		static TextureRef LoadTextureFile(const std::wstring& filePath, eDefaultTexture fallback = kMagenta2D, bool sRGB = false);
		static TextureRef LoadTextureFile(const std::string& filePath, eDefaultTexture fallback = kMagenta2D, bool sRGB = false);


	};
}


