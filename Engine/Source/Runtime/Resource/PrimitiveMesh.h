#pragma once
#include "Runtime/Resource/TextureRef.h"
#include "Runtime/Core/Math/MathInclude.h"
#include "Runtime/Platform/DirectX12/Buffer/UploadBuffer.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include <cstdint>

namespace AtomEngine
{
	enum class PrimitiveMeshType : uint8_t
	{
		Quad = 0,
		Cube,
		Sphere,
		Count
	};

	class PrimitiveMesh
	{
	public:

		PrimitiveMesh(const TextureRef& texture);
		~PrimitiveMesh() = default;

		void Render(const Camera& camera);

		void SetColor(const Color& color) { mColor = color; }
		const Color& GetColor() const { return mColor; }
		Color& GetColor() { return mColor; }
		Transform& GetTransform() { return mTransform; }
		Transform& GetUVTransform() { return mUVTransform; }

		const Matrix4x4 GetWorldMatrix() const { return mTransform.GetMatrix(); }
		const Matrix4x4 GetUVTransformMatrix() const { return mUVTransform.GetMatrix(); }

		void SetTexture(const TextureRef& texture) { mTexture = texture; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetTexture() const { return mTexture.GetSRV(); }
		void SetBlendMode(BlendMode mode) { mBlendMode = mode; }
		BlendMode GetBlendMode() const { return mBlendMode; }
		uint64_t GetSortKey()const;

		void SetVisid(bool flag) { isValid = flag; }
		bool IsValid() const { return isValid; }

		void SetPrimitiveType(PrimitiveMeshType type) { mPrimitiveType = type; }
		PrimitiveMeshType GetPrimitiveType() const { return mPrimitiveType; }

	private:
		Transform mTransform;
		Transform mUVTransform;
		Color mColor = Color::White;
		TextureRef mTexture;
		BlendMode mBlendMode = BlendMode::kBlendNormal;
		float toCameraDistance = 0.0f;
		bool isValid = true;
		PrimitiveMeshType mPrimitiveType = PrimitiveMeshType::Quad;
	};

}
