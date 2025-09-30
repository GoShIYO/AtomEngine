#pragma once
#include "Texture.h"
#include "TexUtil.h"

#include <string>

#include "../Platform/DirectX12/Core/GraphicsCommon.h"

namespace AtomEngine
{
    // テクスチャへの参照カウントポインタ
    class TextureRef;
    // テクスチャファイルの読み込みシステム。
    // テクスチャを共有できるように、テクスチャへの参照が渡されます。
    // テクスチャへのすべての参照が期限切れになると、テクスチャメモリが解放されます。
    class TextureManager
    { 
    public:

        static void Initialize(const std::wstring& RootPath);
        static void Finalize();

        static void DestroyTexture(const std::wstring& key);

        // DDSファイルからテクスチャをロードします。null参照を返すことはありませんが、
        // テクスチャが見つからない場合は、ref->IsValid()はfalseを返します。
        static TextureRef LoadDDSFromFile(const std::wstring& filePath, eDefaultTexture fallback = kMagenta2D, bool sRGB = false);
        static TextureRef LoadDDSFromFile(const std::string& filePath, eDefaultTexture fallback = kMagenta2D, bool sRGB = false);                                                                    
    };

    //前方宣言。プライベートで実装
    class ManagedTexture;

    // ManagedTexture へのハンドル。コンストラクタとデストラクタは参照カウントを変更します。
    // 最後の参照が破棄されると、TextureManager にテクスチャを削除する必要があることが通知されます。
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


