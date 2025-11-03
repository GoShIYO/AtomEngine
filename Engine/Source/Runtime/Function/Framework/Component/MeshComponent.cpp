#include "MeshComponent.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"
#include "Runtime/Function/Render/RenderSystem.h"

namespace AtomEngine
{
	MeshComponent::MeshComponent(std::shared_ptr<const Model>model)
	{
		if (!model)
		{
			mMeshConstantsCPU.Destroy();
			mMeshConstantsGPU.Destroy();
			mSkeletonTransforms.clear();
			mBoundingSphereTransforms.clear();
		}
		else
		{
			mModel = model;

			uint32_t numSceneNode = (uint32_t)mModel->mSceneGraph.size();
			mMeshConstantsCPU.Create(L"Mesh Constants Upload", numSceneNode * sizeof(MeshConstants));
			mMeshConstantsGPU.Create(L"Mesh Constants GPU", numSceneNode, sizeof(MeshConstants));

			mBoundingSphereTransforms.resize(numSceneNode);
			mSkeletonTransforms.resize(mModel->mSkeleton.joints.size());

			if (!model->mAnimationData.empty())
			{
				//TODO : animation
			}
			else
			{

			}
		}
	}

	MeshComponent::~MeshComponent()
	{
		mMeshConstantsCPU.Destroy();
		mMeshConstantsGPU.Destroy();
	}

	void MeshComponent::Update(GraphicsContext& gfxContext, const TransformComponent& transform, float deltaTime)
	{
		if (!mModel)return;
		constexpr size_t kMaxStackDepth = 32;

		size_t stackIdx = 0;
		Matrix4x4 matrixStack[kMaxStackDepth];
		Matrix4x4 ParentMatrix = transform.GetMatrix();

        MeshConstants* cb = (MeshConstants*)mMeshConstantsCPU.Map();

        if (!mAnimGraph.empty())
        {

            for (uint32_t i = 0; i < mAnimGraph.size(); ++i)
            {
                GraphNode& node = mAnimGraph[i];

                if (node.staleMatrix)
                {
                    node.staleMatrix = false;
					node.xform = Matrix4x4::MakeScale(node.scale) * Matrix4x4(node.rotation);
                }
            }
        }

        const GraphNode* sceneGraph = mAnimGraph.empty() ? mModel->mSceneGraph.data() : mAnimGraph.data();

		// ノードがメモリに格納される順序。再帰ではなく行列スタックを使用します。
        for (const GraphNode* Node = sceneGraph; ; ++Node)
        {
            Matrix4x4 xform = Node->xform;
            if (!Node->skeletonRoot)
                xform = ParentMatrix * xform;

			// 変換を親の行列と連結し、行列リストを更新します
            {
                MeshConstants& cbv = cb[Node->matrixIdx];
                cbv.worldMatrix = xform;
                cbv.worldInverseTranspose = xform.Inverse().Transpose();

                mBoundingSphereTransforms[Node->matrixIdx] = xform;
            }

			// 次のノードがchildrenになる場合は、親行列を新しい行列に置き換えます
            if (Node->hasChildren)
            {
                if (Node->hasSibling)
                {
                    ASSERT(stackIdx < kMaxStackDepth, "Overflowed the matrix stack");
                    matrixStack[stackIdx++] = ParentMatrix;
                }
                ParentMatrix = xform;
            }
            else if (!Node->hasSibling)
            {
                if (stackIdx == 0)
                    break;

                ParentMatrix = matrixStack[--stackIdx];
            }
        }

        // スケルトンの行列を更新
        for (uint32_t i = 0; i < mModel->mSkeleton.joints.size(); ++i)
        {
			JointXform& joint = mSkeletonTransforms[i];
            joint.posXform = cb[mModel->mSkeleton.joints[i].index].worldMatrix * mModel->mJointIBMs[i];
            joint.nrmXform = joint.posXform.Inverse().Transpose();
        }

        mMeshConstantsCPU.Unmap();

        gfxContext.TransitionResource(mMeshConstantsGPU, D3D12_RESOURCE_STATE_COPY_DEST, true);
        gfxContext.GetCommandList()->CopyBufferRegion(mMeshConstantsGPU.GetResource(), 0, mMeshConstantsCPU.GetResource(), 0, mMeshConstantsCPU.GetBufferSize());
        gfxContext.TransitionResource(mMeshConstantsGPU, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	void MeshComponent::Render(GraphicsContext& gfxContext, const RootSignature& rootSig, const GraphicsPSO& pso, const Matrix4x4& viewProjMat)
	{
		if (!mModel)return;

		MeshConstants& cbv = *(MeshConstants*)mMeshConstantsCPU.Map();
		cbv.worldMatrix = mWorldMatrix * viewProjMat;
		cbv.worldInverseTranspose = viewProjMat.Inverse().Transpose();
		mMeshConstantsCPU.Unmap();

		auto& textureHeap = Renderer::GetTextureHeap();

		gfxContext.TransitionResource(mMeshConstantsGPU, D3D12_RESOURCE_STATE_COPY_DEST, true);
		gfxContext.GetCommandList()->CopyBufferRegion(mMeshConstantsGPU.GetResource(), 0,
			mMeshConstantsCPU.GetResource(), 0, mMeshConstantsCPU.GetBufferSize());
		gfxContext.TransitionResource(mMeshConstantsGPU, D3D12_RESOURCE_STATE_GENERIC_READ);

		gfxContext.SetRootSignature(rootSig);
		gfxContext.SetPipelineState(pso);

		gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, Renderer::GetTextureHeap().GetHeapPointer());

		gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gfxContext.SetVertexBuffer(0, mModel->mVertexBuffer.VertexBufferView());
		gfxContext.SetIndexBuffer(mModel->mIndexBuffer.IndexBufferView());
		gfxContext.SetConstantBuffer(0, mMeshConstantsGPU.GetGpuVirtualAddress());
		gfxContext.SetDynamicDescriptor(1, 0, mModel->mTextures[0].GetSRV());
		
		gfxContext.DrawIndexedInstanced((UINT)mModel->mNumIndices, 1, 0, 0, 0);
	}

	void MeshComponent::Render(RenderQueue& sorter)
	{
		if (mModel != nullptr)
		{
			mModel->Render(sorter, mMeshConstantsGPU, 
				mBoundingSphereTransforms,
				mSkeletonTransforms.data());
		}
	}
}