#include "Model.h"
#include "AssetManager.h"

#include "Runtime/Function/Render/RenderQueue.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"

#include "../Core/Math/Frustum.h"

namespace AtomEngine
{
	void Model::Render(RenderQueue& sorter, const GpuBuffer& meshConstants,
		const std::vector<Matrix4x4> sphereTransforms, const JointXform* skeleton) const
	{
		Render(sorter, meshConstants, mMaterialConstants, sphereTransforms, skeleton);
	}

	void Model::Render(
		RenderQueue& sorter,
		const GpuBuffer& meshConstants,
		const GpuBuffer& materialConstants,
		const std::vector<Matrix4x4> sphereTransforms,
		const JointXform* skeleton)const
	{
		const Frustum& frustum = sorter.GetViewFrustum();
		const Matrix4x4& viewMat = sorter.GetViewMatrix();

		for (size_t meshIdx = 0; meshIdx < mMeshData.size(); ++meshIdx)
		{
			const Mesh& mesh = mMeshData[meshIdx];
			for (uint32_t subIdx = 0; subIdx < mesh.subMeshes.size(); ++subIdx)
			{
				const SubMesh& sub = mesh.subMeshes[subIdx];

				const Matrix4x4& sphereXform = sphereTransforms[sub.meshCbvIndex];
				float scaleXSqr = sphereXform.GetX().LengthSqr();
				float scaleYSqr = sphereXform.GetY().LengthSqr();
				float scaleZSqr = sphereXform.GetZ().LengthSqr();
				float sphereScale = Math::Sqrt(Math::Max(Math::Max(scaleXSqr, scaleYSqr), scaleZSqr));
				auto translate = sphereXform.GetTrans();

				BoundingSphere sphereLS = mBoundingSphere;
				BoundingSphere sphereWS = BoundingSphere(
					sphereLS.GetCenter() * sphereXform,
					sphereScale * sphereLS.GetRadius()
				);
				BoundingSphere sphereVS = BoundingSphere(
					sphereWS.GetCenter() * viewMat,
					sphereWS.GetRadius() + 1.0f
				);

				// TODO: ReverseZ 対応
				//if (!frustum.IntersectSphere(sphereVS))
				//	continue;
				
				// 距離計算。Reverse Z の場合は注意が必要かもしれないが、
				// z はView空間のz座標なので、通常は負値のはず。GetCenter().z はカメラ前方の距離。
				// 単純なソート用距離としては、Z値をそのまま使うか、カメラからの距離を使う。
				// ここではView空間のZ値と半径を足している。
				float distance = sphereVS.GetCenter().z + sphereVS.GetRadius();

				D3D12_GPU_VIRTUAL_ADDRESS meshCBV =
					meshConstants.GetGpuVirtualAddress() + sub.meshCbvIndex * sizeof(MeshConstants);

				D3D12_GPU_VIRTUAL_ADDRESS materialCBV =
					materialConstants.GetGpuVirtualAddress() + sub.materialIndex * sizeof(MaterialConstants);

				sorter.AddMesh(mesh, skeleton,
					mVertexBuffer.VertexBufferView(),
					mIndexBuffer.IndexBufferView(),
					meshCBV, materialCBV, distance, subIdx);
			}
		}
	}

	void Model::Destroy()
	{
		mVertexBuffer.Destroy();
		mIndexBuffer.Destroy();
		mMaterialConstants.Destroy();
		mTextures.clear();
	}
}