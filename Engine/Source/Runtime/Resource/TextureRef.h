#pragma once
#include "Texture.h"
#include "TexUtil.h"
#include <string>

#include "../Platform/DirectX12/Core/GraphicsCommon.h"
#include "../Platform/DirectX12/Core/DescriptorHeap.h"

namespace AtomEngine
{
	// テクスチャへの参照カウントポインタ
	class TextureRef;

	// ManagedTexture を使用すると、複数のスレッドから同じファイルのテクスチャのロードを要求できます。
	// また、テクスチャの参照カウントも保持されるため、参照されなくなったら解放できます。
	// 生のManagedTexture ポインタは公開されません
	class ManagedTexture : public Texture
	{
		friend class TextureRef;
		friend class AssetManager;
	private:
		ManagedTexture(const std::wstring& FileName);

		void WaitForLoad(void) const;
		void CreateFromMemory(std::shared_ptr<std::vector<byte>> memory, eDefaultTexture fallback, bool sRGB);

		bool IsValid(void) const { return mIsValid; }
		void Unload();

		friend ManagedTexture* FindOrLoadTexture(const std::wstring& fileName, eDefaultTexture fallback, bool forceSRGB);

		std::wstring mMapKey;		// 後でマップから削除するため
		bool mIsValid;
		bool mIsLoading;
		size_t mReferenceCount;
	};

	// ManagedTexture へのハンドル。コンストラクタとデストラクタは参照カウントを変更します。
	// 最後の参照が破棄されると、AssetManager にテクスチャを削除する必要があることが通知されます。
	class TextureRef
	{
	public:

		TextureRef(const TextureRef& ref);
		TextureRef(ManagedTexture* tex = nullptr);
		~TextureRef();

		void operator= (std::nullptr_t);
		void operator= (TextureRef& rhs);
		TextureRef& operator=(const TextureRef& rhs);
		TextureRef& operator=(TextureRef&& rhs) noexcept;

		//これが有効なテクスチャ（正常にロードされたもの）を指していることを確認。
		bool IsValid() const;

		// SRV記述子ハンドルを取得。参照が無効な場合、
		// 有効な記述子ハンドル（フォールバックで指定）を返す。
		D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const;

		// テクスチャポインタを取得
		const Texture* Get(void) const;

		const Texture* operator->(void) const;

	private:
		ManagedTexture* mRef;
	};
}


