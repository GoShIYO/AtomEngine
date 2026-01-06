#include "PrimitiveMesh.h"
#include "Runtime/Function/Render/MeshRenderer.h"
#include "Runtime/Core/Math/Math.h"
#include <algorithm>
#include <atomic>

namespace AtomEngine
{
	PrimitiveMesh::PrimitiveMesh(const TextureRef& texture)
	{
		mTexture = texture;
	}

	void PrimitiveMesh::Render(const Camera& camera)
	{
		if (!isValid)return;

		toCameraDistance = camera.GetPosition().Distance(mTransform.transition);

		MeshRenderer::AddObject(this);
	}

	uint64_t PrimitiveMesh::GetSortKey()const
	{

		union FloatUint { float f; uint32_t u; } dist;
		dist.f = std::max(0.0f, toCameraDistance);

		uint64_t key = 0;

		key |= (uint64_t)(~dist.u) << 32;

		key |= (uint64_t)(static_cast<uint32_t>(mBlendMode) & 0xFF) << 24;

		uint64_t texPtr = (uint64_t)mTexture.GetSRV().ptr;
		key |= (texPtr & 0xFFFFFF) << 0;

		static std::atomic<uint16_t> sCounter{ 0 };
		key ^= (uint64_t)(sCounter.fetch_add(1)) & 0xFFFF;

		return key;
	}

}