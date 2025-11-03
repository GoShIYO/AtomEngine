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
        const Frustum& frustum = sorter.GetViewFrustum();
        const Matrix4x4& viewMat = sorter.GetViewMatrix();

        for (size_t meshIdx = 0; meshIdx < mMeshData.size(); ++meshIdx)
        {
            const Matrix4x4& sphereXform = sphereTransforms[meshIdx];
            float scaleXSqr = sphereXform.GetX().LengthSqr();
            float scaleYSqr = sphereXform.GetY().LengthSqr();
            float scaleZSqr = sphereXform.GetZ().LengthSqr();
            float sphereScale = Math::Sqrt(std::max({ scaleXSqr, scaleYSqr, scaleZSqr }));

            BoundingSphere sphereLS = mBoundingSphere;
            BoundingSphere sphereWS = BoundingSphere(sphereXform * sphereLS.GetCenter(), sphereScale * sphereLS.GetRadius());
            BoundingSphere sphereVS = BoundingSphere(viewMat * sphereWS.GetCenter(), sphereWS.GetRadius());

            //if (!frustum.IntersectSphere(sphereVS))
            //    continue;

            float distance = -sphereVS.GetCenter().z - sphereVS.GetRadius();

            const Mesh& mesh = mMeshData[meshIdx];
            for (uint32_t subIdx = 0; subIdx < mesh.subMeshes.size(); ++subIdx)
            {
                const SubMesh& sub = mesh.subMeshes[subIdx];
                //if (!frustum.IntersectBoundingBox(sub.bounds))
                //    continue;

                D3D12_GPU_VIRTUAL_ADDRESS meshCBV =
                    meshConstants.GetGpuVirtualAddress() + meshIdx * sizeof(MeshConstants);

                D3D12_GPU_VIRTUAL_ADDRESS materialCBV =
                    mMaterialConstants.GetGpuVirtualAddress() + sub.materialIndex * sizeof(MaterialConstants);

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