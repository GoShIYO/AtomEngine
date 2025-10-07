#include "Model.h"
#include "Runtime/Function/Render/RenderQueue.h"

#include "../Platform/DirectX12/Buffer/ConstantBuffers.h"
#include "../Core/Math/Frustum.h"

namespace AtomEngine
{

    void Model::Destroy()
    {
        mDataBuffer.Destroy();
        mMaterialConstants.Destroy();
        mNumNodes = 0;
        mNumMeshes = 0;
        mMeshData = nullptr;
        mSceneGraph = nullptr;
    }

    void Model::Render(
        RenderQueue& sorter,
        const GpuBuffer& meshConstants,
        const Transform sphereTransforms[],
        const Joint* skeleton) const
    {
        // 現在のメッシュへのポインタ
        const uint8_t* pMesh = mMeshData.get();

        const Frustum& frustum = sorter.GetViewFrustum();
        const Matrix4x4& viewMat = sorter.GetViewMatrix();

        for (uint32_t i = 0; i < mNumMeshes; ++i)
        {
            const Mesh& mesh = *(const Mesh*)pMesh;

            const Transform& sphereXform = sphereTransforms[mesh.meshCBV];
            float scaleXSqr = sphereXform.matrix.GetX().LengthSqr();
            float scaleYSqr = sphereXform.matrix.GetY().LengthSqr();
            float scaleZSqr = sphereXform.matrix.GetZ().LengthSqr();
            float sphereScale = Math::Sqrt(std::max(std::max(scaleXSqr, scaleYSqr), scaleZSqr));

            BoundingSphere sphereLS((const Vector4&)mesh.bounds);
            BoundingSphere sphereWS = BoundingSphere(sphereXform * sphereLS.GetCenter(), sphereScale * sphereLS.GetRadius());
            BoundingSphere sphereVS = BoundingSphere(viewMat * sphereWS.GetCenter(), sphereWS.GetRadius());

            if (frustum.IntersectSphere(sphereVS))
            {
                float distance = -sphereVS.GetCenter().z - sphereVS.GetRadius();
                sorter.AddMesh(mesh, distance,
                    meshConstants.GetGpuVirtualAddress() + sizeof(MeshConstants) * mesh.meshCBV,
                    mMaterialConstants.GetGpuVirtualAddress() + sizeof(MaterialConstants) * mesh.materialCBV,
                    mDataBuffer.GetGpuVirtualAddress(), skeleton);
            }

            pMesh += sizeof(Mesh) + (mesh.numDraws - 1) * sizeof(DrawCall);
        }
    }

    void ModelInstance::Render(RenderQueue& sorter) const
    {
        if (m_Model != nullptr)
        {
            m_Model->Render(sorter, m_MeshConstantsGPU, m_BoundingSphereTransforms.get(),
                m_Skeleton.get());
        }
    }

    void ModelInstance::PlayAnimation(uint32_t animIdx, bool loop)
    {
    }

    void ModelInstance::PauseAnimation(uint32_t animIdx)
    {
    }

    void ModelInstance::ResetAnimation(uint32_t animIdx)
    {
    }

    void ModelInstance::StopAnimation(uint32_t animIdx)
    {
    }

    void ModelInstance::UpdateAnimations(float deltaTime)
    {
    }

    void ModelInstance::LoopAllAnimations()
    {
    }

    ModelInstance::ModelInstance(std::shared_ptr<const Model> sourceModel)
        : m_Model(sourceModel)
    {
        static_assert((_alignof(MeshConstants) & 255) == 0, "CBVs need 256 byte alignment");
        if (sourceModel == nullptr)
        {
            m_MeshConstantsCPU.Destroy();
            m_MeshConstantsGPU.Destroy();
            m_BoundingSphereTransforms = nullptr;
            m_AnimGraph = nullptr;
            m_AnimState.clear();
            m_Skeleton = nullptr;
        }
        else
        {
            m_MeshConstantsCPU.Create(L"Mesh Constant Upload Buffer", sourceModel->mNumNodes * sizeof(MeshConstants));
            m_MeshConstantsGPU.Create(L"Mesh Constant GPU Buffer", sourceModel->mNumNodes, sizeof(MeshConstants));

            m_BoundingSphereTransforms.reset(new Transform[sourceModel->mNumNodes]);
            m_Skeleton.reset(new Joint[sourceModel->mNumJoints]);

            if (sourceModel->mNumAnimations > 0)
            {
                m_AnimGraph.reset(new GraphNode[sourceModel->mNumNodes]);
                std::memcpy(m_AnimGraph.get(), sourceModel->mSceneGraph.get(), sourceModel->mNumNodes * sizeof(GraphNode));
                m_AnimState.resize(sourceModel->mNumAnimations);
            }
            else
            {
                m_AnimGraph = nullptr;
                m_AnimState.clear();
            }
        }
    }

    ModelInstance::ModelInstance(const ModelInstance& modelInstance)
        : ModelInstance(modelInstance.m_Model)
    {
    }

    ModelInstance& ModelInstance::operator=(std::shared_ptr<const Model> sourceModel)
    {
        m_Model = sourceModel;

        if (sourceModel == nullptr)
        {
            m_MeshConstantsCPU.Destroy();
            m_MeshConstantsGPU.Destroy();
            m_BoundingSphereTransforms = nullptr;
            m_AnimGraph = nullptr;
            m_AnimState.clear();
            m_Skeleton = nullptr;
        }
        else
        {
            m_MeshConstantsCPU.Create(L"Mesh Constant Upload Buffer", sourceModel->mNumNodes * sizeof(MeshConstants));
            m_MeshConstantsGPU.Create(L"Mesh Constant GPU Buffer", sourceModel->mNumNodes, sizeof(MeshConstants));

            m_BoundingSphereTransforms.reset(new Transform[sourceModel->mNumNodes]);
            m_Skeleton.reset(new Joint[sourceModel->mNumJoints]);

            if (sourceModel->mNumAnimations > 0)
            {
                m_AnimGraph.reset(new GraphNode[sourceModel->mNumNodes]);
                std::memcpy(m_AnimGraph.get(), sourceModel->mSceneGraph.get(), sourceModel->mNumNodes * sizeof(GraphNode));
                m_AnimState.resize(sourceModel->mNumAnimations);
            }
            else
            {
                m_AnimGraph = nullptr;
                m_AnimState.clear();
            }
        }
        return *this;
    }

    void ModelInstance::Update(GraphicsContext& gfxContext, float deltaTime)
    {
        if (m_Model == nullptr)
            return;

        static const size_t kMaxStackDepth = 32;

        size_t stackIdx = 0;
        Matrix4x4 matrixStack[kMaxStackDepth];
        Matrix4x4 ParentMatrix = m_Locator.GetMatrix();

        MeshConstants* cb = (MeshConstants*)m_MeshConstantsCPU.Map();

        if (m_AnimGraph)
        {
            UpdateAnimations(deltaTime);

            for (uint32_t i = 0; i < m_Model->mNumNodes; ++i)
            {
                GraphNode& node = m_AnimGraph[i];

                if (node.staleMatrix)
                {
                    node.staleMatrix = false;
                    node.transform.matrix = (Matrix4x4(node.transform.rotation) * Matrix4x4::MakeScale(node.transform.scale));
                }
            }
        }

        const GraphNode* sceneGraph = m_AnimGraph ? m_AnimGraph.get() : m_Model->mSceneGraph.get();

        // シーングラフを深さ優先で走査する
        // ノードがメモリに格納される順序
        for (const GraphNode* Node = sceneGraph; ; ++Node)
        {
            Matrix4x4 xform = Node->transform.matrix;
            if (!Node->skeletonRoot)
                xform = ParentMatrix * xform;

            // // 変換を親の行列と連結し、行列リストを更新します
            {
                // 書き込み結合メモリを指していることを忘れないようにスコープを設定します。
                // そこから読み取ってはいけません。
                MeshConstants& cbv = cb[Node->matrixIndex];
                cbv.worldMatrix = xform;
                cbv.worldInverseTranspose = xform.Inverse().Transpose();

                m_BoundingSphereTransforms[Node->matrixIndex] = Transform(
                    (Vector3)xform.GetX(),
                    (Vector3)xform.GetY(),
                    (Vector3)xform.GetZ(),
                    (Vector3)xform.GetW());
            }

            //次のノードが子孫になる場合は、親行列を新しい行列に置き換えます。
            if (Node->hasChildren)
            {
                // 現在の親マトリックスをスタックにバックアップ
                if (Node->hasSibling)
                {
                    ASSERT(stackIdx < kMaxStackDepth, "Overflowed the matrix stack");
                    matrixStack[stackIdx++] = ParentMatrix;
                }
                ParentMatrix = xform;
            }
            else if (!Node->hasSibling)
            {
                // スタックが空であれば終了
                // そうでなければ、スタックから行列をポップして処理を続けます
                if (stackIdx == 0)
                    break;

                ParentMatrix = matrixStack[--stackIdx];
            }
        }

        // スケルトンを更新
        for (uint32_t i = 0; i < m_Model->mNumJoints; ++i)
        {
            Joint& joint = m_Skeleton[i];
            joint.posTransform = cb[m_Model->mJointIndices[i]].worldMatrix * m_Model->mJointIBMs[i];
            joint.nrmTransform = joint.nrmTransform.Inverse().Transpose();
        }

        m_MeshConstantsCPU.Unmap();

        gfxContext.TransitionResource(m_MeshConstantsGPU, D3D12_RESOURCE_STATE_COPY_DEST, true);
        gfxContext.GetCommandList()->CopyBufferRegion(m_MeshConstantsGPU.GetResource(), 0, m_MeshConstantsCPU.GetResource(), 0, m_MeshConstantsCPU.GetBufferSize());
        gfxContext.TransitionResource(m_MeshConstantsGPU, D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    void ModelInstance::Resize(float newRadius)
    {
        if (m_Model == nullptr)
            return;

        m_Locator.SetScale(newRadius / m_Model->m_BoundingSphere.GetRadius());
    }

    Vector3 ModelInstance::GetCenter() const
    {
        if (m_Model == nullptr)
            return Vector3::ZERO;

        return m_Locator * m_Model->m_BoundingSphere.GetCenter();
    }

    float ModelInstance::GetRadius() const
    {
        if (m_Model == nullptr)
            return 0.0f;

        return m_Locator.GetScale() * m_Model->m_BoundingSphere.GetRadius();
    }

    BoundingSphere ModelInstance::GetBoundingSphere() const
    {
        if (m_Model == nullptr)
            return BoundingSphere();

        return m_Locator * m_Model->m_BoundingSphere;
    }

    OrientedBox ModelInstance::GetBoundingBox() const
    {
        if (m_Model == nullptr)
            return AxisAlignedBox();

        return m_Locator * m_Model->m_BoundingBox;
    }
}