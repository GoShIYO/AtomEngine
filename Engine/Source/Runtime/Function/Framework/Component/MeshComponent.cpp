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
			mBoundingSphereTransforms.clear();

			mSkeletonTransforms.reset();
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
			mSkeletonTransforms.reset(new JointXform[mModel->mNumJoints]);

			if (!model->mAnimationData.empty())
			{
				mAnimGraph.reset(new GraphNode[numSceneNode]);
				std::memcpy(mAnimGraph.get(), model->mSceneGraph.data(), model->mSceneGraph.size() * sizeof(GraphNode));
				mAnimState.resize(model->mAnimationData.size());
				LoopAllAnimations();
			}
			else
			{
				mAnimGraph.reset();
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
			materialCB.FresnelF0 = material.mMaterials[i].mFresnelF0;
			materialCB.uvTransform = material.mMaterials[i].uvTransform.GetMatrix();
			materialCB.uselight = material.mMaterials[i].useLight;
		}
		mMaterialConstantsCPU.Unmap();

		if (mAnimGraph)
		{
			UpdateAnimation(deltaTime);
			for (uint32_t i = 0; i < mModel->mSceneGraph.size(); ++i)
			{
				GraphNode& node = mAnimGraph[i];

				if (node.staleMatrix)
				{
					node.staleMatrix = false;
					node.xform = Matrix4x4::MakeScale(node.scale) * Matrix4x4(node.rotation);
				}
			}
		}

		const GraphNode* sceneGraph = mAnimGraph ? mAnimGraph.get() : mModel->mSceneGraph.data();

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

		for (uint32_t i = 0; i < mModel->mNumJoints; ++i)
		{
			Joint& joint = mModel->mSkeleton.joints[i];
			joint.localMatrix = joint.transform.GetMatrix();
			if (joint.parent)
			{
				joint.skeletonSpaceMatrix = joint.localMatrix * mModel->mSkeleton.joints[*joint.parent].skeletonSpaceMatrix;
			}
			else
			{
				joint.skeletonSpaceMatrix = joint.localMatrix;
			}

			JointXform& jointTrans = mSkeletonTransforms[i];
			jointTrans.posXform = mModel->mJointIBMs[i] * mModel->mSkeleton.joints[i].skeletonSpaceMatrix;
			jointTrans.nrmXform = Math::InverseTranspose(jointTrans.posXform);
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
			mModel->Render(sorter, mMeshConstantsGPU, mMaterialConstantsGPU,
				mBoundingSphereTransforms,
				mSkeletonTransforms.get());
		}
	}
	void MeshComponent::UpdateAnimation(float deltaTime)
	{
		for (uint32_t i = 0; i < mAnimState.size(); ++i)
		{
			AnimationState& animState = mAnimState[i];
			if (animState.state == AnimationState::kStopped)
				continue;
			animState.time += deltaTime * mDeltaScale;
			const AnimationClip& clip = mModel->mAnimationData[i];

			if (animState.state == AnimationState::kLooping)
				animState.time = std::fmodf(animState.time, clip.duration);
			else if (animState.time > clip.duration)
			{
				animState.time = clip.duration;
				animState.state = AnimationState::kStopped;
			}

			auto& time = animState.time;
			for (auto& curve : clip.curves)
			{
				auto& joint = mModel->mSkeleton.joints[curve.targetJoint];
				joint.transform.scale = CalculateValue(curve.scale, time);
				joint.transform.rotation = CaculateRotation(curve.rotation, time);
				joint.transform.transition = CalculateValue(curve.translation, time);
			}
		}
	}
	void MeshComponent::PlayAnimation(uint32_t animIdx, bool loop)
	{
		if (animIdx >= mAnimState.size())return;

		for(uint32_t i = 0; i < mAnimState.size() ; i++)
		{
			if (i == animIdx)continue;
			auto& animState = mAnimState[i];
			animState.state = AnimationState::kStopped;
			animState.time = 0.0f;
		}
	
		mAnimState[animIdx].state = loop ? AnimationState::kLooping : AnimationState::kPlaying;
	}

	void MeshComponent::PauseAnimation(uint32_t animIdx)
	{
		if (animIdx < mAnimState.size())
			mAnimState[animIdx].state = AnimationState::kStopped;
	}

	void MeshComponent::ResetAnimation(uint32_t animIdx)
	{
		if (animIdx < mAnimState.size())
			mAnimState[animIdx].time = 0.0f;
	}

	void MeshComponent::StopAnimation(uint32_t animIdx)
	{
		if (animIdx < mAnimState.size())
		{
			mAnimState[animIdx].state = AnimationState::kStopped;
			mAnimState[animIdx].time = 0.0f;
		}
	}

	void MeshComponent::LoopAllAnimations()
	{
		for (auto& anim : mAnimState)
		{
			anim.state = AnimationState::kLooping;
			anim.time = 0.0f;
		}
	}
}
