#include "TextureManager.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"
#include "Runtime/Platform/DirectX12/Context/CommandContext.h"
#include "DirectXTex.h"
#include <map>
#include <mutex>

namespace AtomEngine
{

	// ManagedTexture を使用すると、複数のスレッドから同じファイルのテクスチャのロードを要求できます。
	// また、テクスチャの参照カウントも保持されるため、参照されなくなったら解放できます。
	// 生の ManagedTexture ポインタはクライアントには公開されません。

	class ManagedTexture : public Texture
	{
		friend class TextureRef;
		friend class TextureManager;
	public:
		ManagedTexture(const std::wstring& FileName);

		void WaitForLoad(void) const;
		void CreateFromMemory(std::shared_ptr<std::vector<byte>> memory, eDefaultTexture fallback, bool sRGB);

	private:

		bool IsValid(void) const { return mIsValid; }
		void Unload();

		std::wstring mMapKey;		// 後でマップから削除するため
		bool mIsValid;
		bool mIsLoading;
		size_t mReferenceCount;
	};

	std::wstring sRootPath = L"";
	std::map<std::wstring, std::unique_ptr<ManagedTexture>>sTextureCache;
	std::mutex sMutex;

	void TextureManager::Initialize(const std::wstring& rootPath)
	{
		sRootPath = rootPath;
	}

	void TextureManager::Finalize()
	{
		sTextureCache.clear();
	}

	void TextureManager::DestroyTexture(const std::wstring& key)
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

	ManagedTexture::ManagedTexture(const std::wstring& FileName)
		: mMapKey(FileName), mIsValid(false), mIsLoading(true), mReferenceCount(0)
	{
		mCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	void ManagedTexture::CreateFromMemory(std::shared_ptr<std::vector<byte>> ba, eDefaultTexture fallback, bool forceSRGB)
	{
		if (ba->empty())
		{
			mCpuDescriptorHandle = GetDefaultTexture(fallback);
			mIsLoading = false;
			return;
		}

		mCpuDescriptorHandle = DX12Core::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		DirectX::TexMetadata metadata = {};
		DirectX::ScratchImage scratch = {};

		HRESULT hr = DirectX::LoadFromDDSMemory(ba->data(), ba->size(), DirectX::DDS_FLAGS_NONE, &metadata, scratch);
		if (FAILED(hr))
		{
			DX12Core::gDevice->CopyDescriptorsSimple(1, mCpuDescriptorHandle,
				GetDefaultTexture(fallback), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			return;
		}

		if (forceSRGB) metadata.format = DirectX::MakeSRGB(metadata.format);

		hr = DirectX::CreateTexture(DX12Core::gDevice.Get(), metadata, mResource.GetAddressOf());
		if (FAILED(hr))
		{
			DX12Core::gDevice->CopyDescriptorsSimple(1, mCpuDescriptorHandle,
				GetDefaultTexture(fallback), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			return;
		}

		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		hr = DirectX::PrepareUpload(DX12Core::gDevice.Get(),
			scratch.GetImages(), scratch.GetImageCount(), metadata, subresources);
		if (FAILED(hr)) return;

		GpuResource gpuTex(mResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
		CommandContext::InitializeTexture(gpuTex, (UINT)subresources.size(), subresources.data());

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = metadata.format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_UNKNOWN;

		switch (metadata.dimension)
		{
		case DirectX::TEX_DIMENSION_TEXTURE1D:
			if (metadata.arraySize > 1)
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
				srvDesc.Texture1DArray.MipLevels = (UINT)metadata.mipLevels;
			}
			else
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
				srvDesc.Texture1D.MipLevels = (UINT)metadata.mipLevels;
			}
			break;

		case DirectX::TEX_DIMENSION_TEXTURE2D:
			if (metadata.IsCubemap())
			{
				if (metadata.arraySize > 6)
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
					srvDesc.TextureCubeArray.MipLevels = (UINT)metadata.mipLevels;
				}
				else
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
					srvDesc.TextureCube.MipLevels = (UINT)metadata.mipLevels;
				}
			}
			else
			{
				if (metadata.arraySize > 1)
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
					srvDesc.Texture2DArray.MipLevels = (UINT)metadata.mipLevels;
				}
				else
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.MipLevels = (UINT)metadata.mipLevels;
				}
			}
			break;

		case DirectX::TEX_DIMENSION_TEXTURE3D:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MipLevels = (UINT)metadata.mipLevels;
			break;
		}

		DX12Core::gDevice->CreateShaderResourceView(mResource.Get(), &srvDesc, mCpuDescriptorHandle);

		mUsageState = D3D12_RESOURCE_STATE_GENERIC_READ;
		mIsValid = true;
		mWidth = (uint32_t)metadata.width;
		mHeight = (uint32_t)metadata.height;
		mDepth = (uint32_t)metadata.depth;
		mIsLoading = false;
	}

	void ManagedTexture::WaitForLoad(void) const
	{
		while ((volatile bool&)mIsLoading)
			std::this_thread::yield();
	}

	void ManagedTexture::Unload()
	{
		TextureManager::DestroyTexture(mMapKey);
	}

	TextureRef::TextureRef(const TextureRef& ref) : mRef(ref.mRef)
	{
		if (mRef != nullptr)
			++mRef->mReferenceCount;
	}

	TextureRef::TextureRef(ManagedTexture* tex) : mRef(tex)
	{
		if (mRef != nullptr)
			++mRef->mReferenceCount;
	}

	TextureRef::~TextureRef()
	{
		if (mRef != nullptr && --mRef->mReferenceCount == 0)
			mRef->Unload();
	}

	void TextureRef::operator= (std::nullptr_t)
	{
		if (mRef != nullptr)
			--mRef->mReferenceCount;

		mRef = nullptr;
	}

	void TextureRef::operator= (TextureRef& rhs)
	{
		if (mRef != nullptr)
			--mRef->mReferenceCount;

		mRef = rhs.mRef;

		if (mRef != nullptr)
			++mRef->mReferenceCount;
	}
	TextureRef& TextureRef::operator=(const TextureRef& rhs)
	{
		if (this == &rhs)
			return *this;

		if (mRef != nullptr)
		{
			--mRef->mReferenceCount;
			if (mRef->mReferenceCount == 0)
				mRef->Unload();
		}

		mRef = rhs.mRef;

		if (mRef != nullptr)
			++mRef->mReferenceCount;

		return *this;
	}

	TextureRef& TextureRef::operator=(TextureRef&& rhs) noexcept
	{
		if (this == &rhs)
			return *this;

		if (mRef != nullptr)
		{
			--mRef->mReferenceCount;
			if (mRef->mReferenceCount == 0)
				mRef->Unload();
		}

		mRef = rhs.mRef;
		rhs.mRef = nullptr;

		return *this;
	}

	bool TextureRef::IsValid() const
	{
		return mRef && mRef->IsValid();
	}

	const Texture* TextureRef::Get(void) const
	{
		return mRef;
	}

	const Texture* TextureRef::operator->(void) const
	{
		ASSERT(mRef != nullptr);
		return mRef;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE TextureRef::GetSRV() const
	{
		if (mRef != nullptr)
			return mRef->GetSRV();
		else
			return GetDefaultTexture(kMagenta2D);
	}

	TextureRef TextureManager::LoadDDSFromFile(const std::wstring& filePath, eDefaultTexture fallback, bool forceSRGB)
	{
		return FindOrLoadTexture(filePath, fallback, forceSRGB);
	}

	TextureRef TextureManager::LoadDDSFromFile(const std::string& filePath, eDefaultTexture fallback, bool forceSRGB)
	{
		return LoadDDSFromFile(UTF8ToWString(filePath), fallback, forceSRGB);
	}

}