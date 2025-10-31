#include "Model.h"
#include "AssetManager.h"

#include "Runtime/Function/Render/RenderQueue.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"

#include "../Core/Math/Frustum.h"

namespace AtomEngine
{
    void Model::Render(RenderQueue& sorter, const GpuBuffer& meshConstants,
        const std::vector<Transform> sphereTransforms, const JointXform* skeleton) const
    {
        const uint8_t* pMesh = mMeshData.data();
        const Frustum& frustum = sorter.GetViewFrustum();
        const Matrix4x4& viewMat = sorter.GetViewMatrix();

        for (uint32_t meshIdx = 0; meshIdx < mNumMeshs; ++meshIdx)
        {
            const Mesh& mesh = *(const Mesh*)pMesh;
            pMesh += sizeof(Mesh);

            const Matrix4x4& sphereXform = sphereTransforms[meshIdx].GetMatrix();
            float scaleXSqr = sphereXform.GetX().LengthSqr();
            float scaleYSqr = sphereXform.GetY().LengthSqr();
            float scaleZSqr = sphereXform.GetZ().LengthSqr();
            float sphereScale = Math::Sqrt(std::max({ scaleXSqr, scaleYSqr, scaleZSqr }));

            BoundingSphere sphereLS = mBoundingSphere;
            BoundingSphere sphereWS = BoundingSphere(sphereXform * sphereLS.GetCenter(), sphereScale * sphereLS.GetRadius());
            BoundingSphere sphereVS = BoundingSphere(viewMat * sphereWS.GetCenter(), sphereWS.GetRadius());

            if (!frustum.IntersectSphere(sphereVS))
            {
                pMesh += mesh.subMeshes.size() * sizeof(SubMesh);
                continue;
            }

            float distance = -sphereVS.GetCenter().z - sphereVS.GetRadius();

            for (const SubMesh& sub : mesh.subMeshes)
            {
                if (!frustum.IntersectBoundingBox(sub.bounds))
                    continue;

                D3D12_GPU_VIRTUAL_ADDRESS meshCBV =
                    meshConstants.GetGpuVirtualAddress() + meshIdx * sizeof(MeshConstants);

                D3D12_GPU_VIRTUAL_ADDRESS materialCBV =
                    mMaterialConstants.GetGpuVirtualAddress() + sub.materialIndex * sizeof(MaterialConstants);

                sorter.AddMesh(mesh, skeleton,
                    mVertexBuffer.VertexBufferView(),
                    mIndexBuffer.IndexBufferView(),
                    meshCBV, materialCBV, distance);
            }

            pMesh += mesh.subMeshes.size() * sizeof(SubMesh);
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