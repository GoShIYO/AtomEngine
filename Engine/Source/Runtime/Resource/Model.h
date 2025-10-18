#pragma once
#include "Mesh.h"
#include "Animation.h"
#include "TextureRef.h"
#include "Material.h"

#include "../Platform/DirectX12/Buffer/GpuBuffer.h"
#include "../Platform/DirectX12/Buffer/UploadBuffer.h"
#include "../Platform/DirectX12/Context/GraphicsContext.h"

#include "../Core/Math/BoundingBox.h"
#include "../Core/Math/BoundingSphere.h"

#include <span>

namespace AtomEngine
{
	struct SkinConstants
	{
		Matrix4x4 skeletonMatrix;
		Matrix4x4 skeletonInverseTranspose;
	};

	struct SkinCluster
	{
		std::vector<Matrix4x4> inverseBindPoseMatrices;

		StructuredBuffer paletteResource;
		std::span<SkinConstants> mappedPalette;
		DescriptorHandle paletteSrv;
	};

	class RenderQueue;

	class Model
	{
	public:
		~Model(){Destroy();}

		ByteAddressBuffer vertexBuffer;
		ByteAddressBuffer indexBuffer;
		std::vector<TextureRef> textures;


	protected:

		void Destroy();
	};

	class ModelInstance
	{
	public:
		ModelInstance() {}
		~ModelInstance()
		{
			m_MeshConstantsCPU.Destroy();
			m_MeshConstantsGPU.Destroy();
		}
		ModelInstance(std::shared_ptr<const Model> sourceModel);
		ModelInstance(const ModelInstance& modelInstance);

		ModelInstance& operator=(std::shared_ptr<const Model> sourceModel);

		bool IsNull(void) const { return m_Model == nullptr; }

		void Update(GraphicsContext& gfxContext, float deltaTime);

		BoundingSphere GetBoundingSphere() const;
		OrientedBox GetBoundingBox() const;
		void Render(RenderQueue& sorter) const;

	private:
		std::shared_ptr<const Model> m_Model;
		UploadBuffer m_MeshConstantsCPU;
		ByteAddressBuffer m_MeshConstantsGPU;
		Transform m_Locator;
	};
}


