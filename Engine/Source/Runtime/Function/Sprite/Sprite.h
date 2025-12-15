#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Platform/DirectX12/Pipline/PiplineState.h"
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"
#include "Runtime/Platform/DirectX12/Core/GraphicsCommon.h"
#include "Runtime/Resource/TextureRef.h"

namespace AtomEngine
{	
	struct SpriteVertex
	{ 
		Vector3 position;
		Vector2 uv;
	};

	struct SpriteDesc
	{
		// 座標
		Vector2 position{};
		// スプライト幅、高さ
		Vector2 size {};
		// テクスチャの基点
		Vector2 texBase = { 0, 0 };
		// テクスチャのピクセル幅、高さ
		Vector2 texSize{};
		// アンカーポイント
		Vector2 pivot = { 0, 0 };
		// 色
		Color color = Color::White;
		// 左右反転
		bool isFlipX = false;
		// 上下反転
		bool isFlipY = false;
		// ソートキー
		uint32_t depth = 1;
	};

	class Sprite
	{
	public:
		Sprite(TextureRef texture);
		~Sprite() = default;

		void SetTexture(TextureRef texture) { mTexture = texture; }
		/// <summary>
		///	セットブレンドモード
		/// </summary>
		/// <param name="blendMode">モード</param>
		void SetBlendMode(BlendMode blendMode) { mBlendMode = blendMode; }

		inline void SetColor(Color color_) { mDesc.color = color_; };
		void SetVisible(bool flag) { mIsValid = flag; }
		bool Valid() const{return mIsValid;}

		const Matrix4x4 WorldMatrix()const { return mWorldTransform.GetMatrix(); }
		const Matrix4x4 UVMatrix()const { return mUVTransform.GetMatrix(); }
		BlendMode GetBlendMode() const { return mBlendMode; }
		void SetDesc(const SpriteDesc& desc) { mDesc = desc; }
		const SpriteDesc& GetDesc() const { return mDesc; }
		const TextureRef& GetTexture() const { return mTexture; }

		Transform& GetWorldTransform() { return mWorldTransform; }
		Transform& GetUVTransform() { return mUVTransform; }
		void SetDepth(uint32_t depth) { mDesc.depth = depth; }
		void Render(const Vector2& position,const Vector2& scale,float angle,const Vector2& pivot = {0,0}, const Color& color = Color::White, uint32_t layer = 0);
		void Render(const Vector2& position, const Vector2& scale, float angle, TextureRef texture, const Vector2& pivot = {0,0}, const Color& color = Color::White, uint32_t layer = 0);
		void Render();

	private:
		SpriteDesc mDesc;
		TextureRef mTexture;
		bool mIsValid = true;

		Transform mWorldTransform;
		Transform mUVTransform;
		BlendMode mBlendMode = BlendMode::kBlendNormal;
	};
}

