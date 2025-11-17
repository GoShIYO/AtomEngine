#include "MeshComponent.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"
#include "Runtime/Function/Render/RenderSystem.h"
#include "imgui.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"

namespace AtomEngine
{
	MeshComponent::MeshComponent(std::shared_ptr<Model>model)
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

			mMaterialConstantsCPU.Create(L"Material Constants Upload", (uint32_t)mModel->mMaterials.size() * sizeof(MaterialConstants));
			mMaterialConstantsGPU.Create(L"Material Constants GPU", (uint32_t)mModel->mMaterials.size(), sizeof(MaterialConstants));

			mBoundingSphereTransforms.resize(numSceneNode);
			mSkeletonTransforms.resize(mModel->mSkeleton.joints.size());

			if (!model->mAnimationData.empty())
			{
				mAnimGraph.resize(model->mSceneGraph.size());
				std::memcpy(mAnimGraph.data(), model->mSceneGraph.data(), model->mSceneGraph.size() * sizeof(GraphNode));
				mAnimState.resize(model->mAnimationData.size());
			}
			else
			{
				mAnimGraph.clear();
				mAnimState.clear();
			}
		}
	}

	MeshComponent::~MeshComponent()
	{
		mMeshConstantsCPU.Destroy();
		mMeshConstantsGPU.Destroy();

		mMaterialConstantsCPU.Destroy();
		mMaterialConstantsGPU.Destroy();

		DX12Core::gCommandManager.IdleGPU();
	}

	void MeshComponent::Update(GraphicsContext& gfxContext, const TransformComponent& transform, float deltaTime)
	{
		if (!mModel)return;
		constexpr size_t kMaxStackDepth = 32;

		size_t stackIdx = 0;
		Matrix4x4 matrixStack[kMaxStackDepth];
		Matrix4x4 ParentMatrix = transform.GetMatrix() * mModelTransform.GetMatrix();

		MeshConstants* cb = (MeshConstants*)mMeshConstantsCPU.Map();

		if (!mAnimGraph.empty())
		{
			UpdateAnimation(deltaTime);
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
				xform = xform * ParentMatrix;

			// 変換を親の行列と連結し、行列リストを更新します
			{
				MeshConstants& cbv = cb[Node->matrixIdx];
				cbv.worldMatrix = xform;
				cbv.worldInverseTranspose = Math::InverseTranspose(xform);

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
        gfxContext.TransitionResource(mMaterialConstantsGPU, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	void MeshComponent::Update(GraphicsContext& gfxContext, const TransformComponent& transform, const MaterialComponent& material, float deltaTime)
	{
		if (!mModel)return;
		constexpr size_t kMaxStackDepth = 32;

		size_t stackIdx = 0;
		Matrix4x4 matrixStack[kMaxStackDepth];
		Matrix4x4 ParentMatrix = transform.GetMatrix() * mModelTransform.GetMatrix();

		MeshConstants* cb = (MeshConstants*)mMeshConstantsCPU.Map();
		MaterialConstants* mc = (MaterialConstants*)mMaterialConstantsCPU.Map();
		for (uint32_t i = 0; i < mModel->mMaterials.size(); ++i)
		{
			MaterialConstants& materialCB = mc[i];
			materialCB.baseColorFactor = material.mMaterials[i].mBaseColor;
            materialCB.metallicFactor = material.mMaterials[i].mMetallic;
            materialCB.roughnessFactor = material.mMaterials[i].mRoughness;
			materialCB.normalTextureScale = material.mMaterials[i].mNormalScale;
			materialCB.emissiveFactor = material.mMaterials[i].mEmissive;
		}
		mMaterialConstantsCPU.Unmap();

		if (!mAnimGraph.empty())
		{
			UpdateAnimation(deltaTime);
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
				xform = xform * ParentMatrix;

			// 変換を親の行列と連結し、行列リストを更新します
			{
				MeshConstants& cbv = cb[Node->matrixIdx];
				cbv.worldMatrix = xform;
				cbv.worldInverseTranspose = Math::InverseTranspose(xform);

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
		gfxContext.TransitionResource(mMaterialConstantsGPU, D3D12_RESOURCE_STATE_COPY_DEST, true);
		gfxContext.GetCommandList()->CopyBufferRegion(mMeshConstantsGPU.GetResource(), 0, mMeshConstantsCPU.GetResource(), 0, mMeshConstantsCPU.GetBufferSize());
		gfxContext.GetCommandList()->CopyBufferRegion(mMaterialConstantsGPU.GetResource(), 0, mMaterialConstantsCPU.GetResource(), 0, mMaterialConstantsCPU.GetBufferSize());
		gfxContext.TransitionResource(mMeshConstantsGPU, D3D12_RESOURCE_STATE_GENERIC_READ);
		gfxContext.TransitionResource(mMaterialConstantsGPU, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	void MeshComponent::Render(RenderQueue& sorter)
	{
		if (mModel != nullptr)
		{
			mModel->Render(sorter, mMeshConstantsGPU,mMaterialConstantsGPU,
				mBoundingSphereTransforms,
				mSkeletonTransforms.data());
		}
	}
	void MeshComponent::UpdateAnimation(float deltaTime)
	{
		for (uint32_t i = 0; i < mAnimState.size(); ++i)
		{
			AnimationState& animState = mAnimState[i];
			if (animState.state == AnimationState::kStopped)
				continue;
			animState.time += deltaTime;
			const AnimationClip& clip = mModel->mAnimationData[i];

			if (animState.state == AnimationState::kLooping)
				animState.time = fmodf(animState.time, clip.duration);
			else if (animState.time > clip.duration)
			{
				animState.time = clip.duration;
				animState.state = AnimationState::kStopped;
			}
			auto& time = animState.time;
			for (auto& curve : clip.curves)
			{
				auto& joint = mModel->mSkeleton.joints[curve.targetJoint];
				joint.transform.transition = CalculateValue(curve.translation, time);
				joint.transform.rotation = CaculateRotation(curve.rotation, time);
				joint.transform.scale = CalculateValue(curve.scale, time);
			}
		}
	}
}
