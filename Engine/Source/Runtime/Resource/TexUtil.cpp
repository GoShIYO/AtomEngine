#include "TexUtil.h"
#include "../Core/Utility/Utility.h"
#include <filesystem>

#define GetFlag(f) ((Flags & f) != 0)

using namespace DirectX;

namespace AtomEngine
{
	void CompileTextureOnDemand(const std::wstring& originalFile, uint32_t flags)
	{
		std::wstring ddsFile = RemoveExtension(originalFile) + L".dds";

		bool srcFileExists = std::filesystem::exists(originalFile);
		bool ddsFileExists = std::filesystem::exists(ddsFile);

		if (!srcFileExists && !ddsFileExists)
		{
			Printf("[Info]Texture %ws is missing.\n", RemoveBasePath(originalFile).c_str());
			return;
		}

		std::filesystem::file_time_type srcLastWriteTime;
		std::filesystem::file_time_type ddsLastWriteTime;

		if (srcFileExists)
			srcLastWriteTime = std::filesystem::last_write_time(originalFile);

		if (ddsFileExists)
			ddsLastWriteTime = std::filesystem::last_write_time(ddsFile);

		if (!ddsFileExists || (srcFileExists && ddsLastWriteTime < srcLastWriteTime))
		{
			Printf("[Info]DDS mTexture %ws missing or older than source. Rebuilding.\n", RemoveBasePath(originalFile).c_str());
			ConvertToDDS(originalFile, flags);
		}
	}

	bool ConvertToDDS(const std::wstring& filePath, uint32_t Flags)
	{
		bool bInterpretAsSRGB = GetFlag(kSRGB);
		bool bPreserveAlpha = GetFlag(kPreserveAlpha);
		bool bContainsNormals = GetFlag(kNormalMap);
		bool bBumpMap = GetFlag(kBumpToNormal);
		bool bBlockCompress = GetFlag(kDefaultBC);
		bool bUseBestBC = GetFlag(kQualityBC);
		bool bFlipImage = GetFlag(kFlipVertical);

		ASSERT(!bInterpretAsSRGB || !bContainsNormals);
		ASSERT(!bPreserveAlpha || !bContainsNormals);

		Printf("Converting file \"%ws\" to DDS.\n", filePath.c_str());

		// 拡張子をutf8（ascii）として取得する
		std::wstring ext = ToLower(GetFileExtension(filePath));

		// テクスチャ画像を読み込む
		TexMetadata info;
		std::unique_ptr<ScratchImage> image(new ScratchImage);

		bool isDDS = false;
		bool isHDR = false;
		if (ext == L"dds")
		{
			// TODO: 既存の DDS ファイルを圧縮または再圧縮する必要がある場合があります
			//Printf("Ignoring existing DDS \"%ws\".\n", filePath.c_str());
			//false を返します。

			isDDS = true;
			HRESULT hr = LoadFromDDSFile(filePath.c_str(), DDS_FLAGS_NONE, &info, *image);
			if (FAILED(hr))
			{
				Printf("Could not load mTexture \"%ws\" (DDS: %08X).\n", filePath.c_str(), hr);
				return false;
			}
		}
		else if (ext == L"tga")
		{
			HRESULT hr = LoadFromTGAFile(filePath.c_str(), &info, *image);
			if (FAILED(hr))
			{
				Printf("Could not load mTexture \"%ws\" (TGA: %08X).\n", filePath.c_str(), hr);
				return false;
			}
		}
		else if (ext == L"hdr")
		{
			isHDR = true;
			HRESULT hr = LoadFromHDRFile(filePath.c_str(), &info, *image);
			if (FAILED(hr))
			{
				Printf("Could not load mTexture \"%ws\" (HDR: %08X).\n", filePath.c_str(), hr);
				return false;
			}
		}
		else
		{
			WIC_FLAGS wicFlags = WIC_FLAGS_NONE;
			//if (g_pScene->Settings().bIgnoreSRGB)
			//    wicFlags |= WIC_FLAGS_IGNORE_SRGB;

			HRESULT hr = LoadFromWICFile(filePath.c_str(), wicFlags, &info, *image);
			if (FAILED(hr))
			{
				Printf("Could not load mTexture \"%ws\" (WIC: %08X).\n", filePath.c_str(), hr);
				return false;
			}
		}

		if (info.width > 16384 || info.height > 16384)
		{
			Printf("Texture size (%Iu,%Iu) too large for feature level 11.0 or later (16384) \"%ws\".\n",
				info.width, info.height, filePath.c_str());
			return false;
		}

		if (bFlipImage)
		{
			std::unique_ptr<ScratchImage> timage(new ScratchImage);

			HRESULT hr = FlipRotate(image->GetImages()[0], TEX_FR_FLIP_VERTICAL, *timage);

			if (FAILED(hr))
			{
				Printf("Could not flip image \"%ws\" (%08X).\n", filePath.c_str(), hr);
			}
			else
			{
				image.swap(timage);
			}

		}

		DXGI_FORMAT tformat;
		DXGI_FORMAT cformat;

		if (isHDR)
		{
			tformat = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
			cformat = bBlockCompress ? DXGI_FORMAT_BC6H_UF16 : DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
		}
		else if (bBlockCompress)
		{
			tformat = bInterpretAsSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
			if (bUseBestBC)
				cformat = bInterpretAsSRGB ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;
			else if (bPreserveAlpha)
				cformat = bInterpretAsSRGB ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
			else
				cformat = bInterpretAsSRGB ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
		}
		else
		{
			cformat = tformat = bInterpretAsSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
		}

		if (bBumpMap)
		{
			std::unique_ptr<ScratchImage> timage(new ScratchImage);

			HRESULT hr = ComputeNormalMap(image->GetImages(), image->GetImageCount(), image->GetMetadata(),
				CNMAP_CHANNEL_LUMINANCE, 10.0f, tformat, *timage);

			if (FAILED(hr))
			{
				Printf("Could not compute normal map for \"%ws\" (%08X).\n", filePath.c_str(), hr);
			}
			else
			{
				image.swap(timage);
				info.format = tformat;
			}
		}
		else if (info.format != tformat)
		{
			std::unique_ptr<ScratchImage> timage(new ScratchImage);

			HRESULT hr = Convert(image->GetImages(), image->GetImageCount(), image->GetMetadata(),
				tformat, TEX_FILTER_DEFAULT, 0.5f, *timage);

			if (FAILED(hr))
			{
				Printf("Could not convert \"%ws\" (%08X).\n", filePath.c_str(), hr);
			}
			else
			{
				image.swap(timage);
				info.format = tformat;
			}
		}

		// ミップマップ
		if (info.mipLevels == 1)
		{
			std::unique_ptr<ScratchImage> timage(new ScratchImage);

			HRESULT hr = GenerateMipMaps(image->GetImages(), image->GetImageCount(), image->GetMetadata(), TEX_FILTER_DEFAULT, 0, *timage);

			if (FAILED(hr))
			{
				Printf("Failing generating mimaps for \"%ws\" (WIC: %08X).\n", filePath.c_str(), hr);
			}
			else
			{
				image.swap(timage);
			}
		}

		// ハンドル圧縮
		if (bBlockCompress)
		{
			if (info.width % 4 || info.height % 4)
			{
				Printf("Texture size (%Iux%Iu) not a multiple of 4 \"%ws\", so skipping compress\n", info.width, info.height, filePath.c_str());
			}
			else
			{
				std::unique_ptr<ScratchImage> timage(new ScratchImage);

				HRESULT hr = Compress(image->GetImages(), image->GetImageCount(), image->GetMetadata(), cformat, TEX_COMPRESS_DEFAULT, 0.5f, *timage);
				if (FAILED(hr))
				{
					Printf("Failing compressing \"%ws\" (WIC: %08X).\n", filePath.c_str(), hr);
				}
				else
				{
					image.swap(timage);
				}
			}
		}

		// ファイル拡張子をDDSに変更する
		const std::wstring wDest = RemoveExtension(filePath) + L".dds";

		// DDSを保存
		HRESULT hr = SaveToDDSFile(image->GetImages(), image->GetImageCount(), image->GetMetadata(), DDS_FLAGS_NONE, wDest.c_str());
		if (FAILED(hr))
		{
			Printf("Could not write mTexture to file \"%ws\" (%08X).\n", wDest.c_str(), hr);
			return false;
		}

		return true;
	}

}

