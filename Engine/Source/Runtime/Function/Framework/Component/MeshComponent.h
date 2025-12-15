#pragma once
#include "TransformComponent.h"
#include "MaterialComponent.h"
#include "Runtime/Platform/DirectX12/Buffer/GpuBuffer.h"
#include "Runtime/Platform/DirectX12/Buffer/UploadBuffer.h"
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"
#include "Runtime/Platform/DirectX12/Pipline/PiplineState.h"
#include<memory>

namespace AtomEngine
{

	class MeshComponent
	{
	public:
		MeshComponent(std::shared_ptr<Model> model);
		~MeshComponent();

		void Update(GraphicsContext& gfxContext, const TransformComponent& transform, const MaterialComponent& material, float deltaTime);
		void Render(RenderQueue& sorter);

		Transform& GetTransform() { return mModelTransform; }

		void UpdateAnimation(float deltaTime);

		void PlayAnimation(uint32_t animIdx, bool loop);

		void PauseAnimation(uint32_t animIdx);

		void ResetAnimation(uint32_t animIdx);

		void StopAnimation(uint32_t animIdx);

		void LoopAllAnimations();
		uint32_t GetNumAnimations() const { return static_cast<uint32_t>(mAnimState.size()); }
		bool IsRender() const { return mIsRender; }
		void SetIsRender(bool isRender) { mIsRender = isRender; }
		void SetDeltaScale(float deltaScale) { mDeltaScale = deltaScale; }
	private:
		bool mIsRender = true;
		std::shared_ptr<Model> mModel = nullptr;
		UploadBuffer mMeshConstantsCPU;
		ByteAddressBuffer mMeshConstantsGPU;

		UploadBuffer mMaterialConstantsCPU;
		ByteAddressBuffer mMaterialConstantsGPU;

		Transform mModelTransform;
		std::unique_ptr <GraphNode[]> mAnimGraph;
		std::vector<AnimationState> mAnimState;

		std::unique_ptr<JointXform[]> mSkeletonTransforms;
		std::vector<Matrix4x4> mBoundingSphereTransforms;
		float mDeltaScale = 1.0f;
	};
}


