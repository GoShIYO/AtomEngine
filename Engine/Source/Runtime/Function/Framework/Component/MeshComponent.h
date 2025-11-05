#pragma once
#include "TransformComponent.h"
#include "Runtime/Resource/Model.h"
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

		void Update(GraphicsContext& gfxContext,const TransformComponent& transform ,float deltaTime);

		void Render(RenderQueue& sorter);

		void UpdateAnimation(float deltaTime);
	private:
		std::shared_ptr<Model> mModel = nullptr;
		UploadBuffer mMeshConstantsCPU;
		ByteAddressBuffer mMeshConstantsGPU;
		
		Matrix4x4 mWorldMatrix;

		std::vector<GraphNode> mAnimGraph;
		std::vector<AnimationState> mAnimState;

		std::vector<JointXform> mSkeletonTransforms;
		std::vector<Matrix4x4> mBoundingSphereTransforms;
	};
}


