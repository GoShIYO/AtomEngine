#pragma once
#include "Mesh.h"
#include "Animation.h"
#include "TextureRef.h"
#include "Material.h"
#include "Skeleton.h"

#include "../Platform/DirectX12/Buffer/GpuBuffer.h"

#include "../Core/Math/BoundingBox.h"
#include "../Core/Math/BoundingSphere.h"

#include <map>

namespace AtomEngine
{
	class RenderQueue;

	struct JointXform
	{
		Matrix4x4 posXform;
		Matrix4x4 nrmXform;
	};

	struct GraphNode
	{
		Matrix4x4 xform;
		Quaternion rotation;
        Vector3 scale;

		uint32_t matrixIdx : 28;
		uint32_t hasSibling : 1;
		uint32_t hasChildren : 1;
		uint32_t staleMatrix : 1;
		uint32_t skeletonRoot : 1;
	};

	class Model
	{
	public:
		~Model(){Destroy();}
		
		void Render(RenderQueue& sorter,
			const GpuBuffer& meshConstants,
			const std::vector<Matrix4x4> sphereTransforms,
			const JointXform* skeleton) const;

		void Render(RenderQueue& sorter,
			const GpuBuffer& meshConstants,
			const GpuBuffer& materialConstants,
			const std::vector<Matrix4x4> sphereTransforms,
			const JointXform* skelton)const;

		BoundingSphere mBoundingSphere;
		AxisAlignedBox mBoundingBox;

		ByteAddressBuffer mVertexBuffer;
		ByteAddressBuffer mIndexBuffer;
		ByteAddressBuffer mMaterialConstants;

		std::vector<Material> mMaterials;
		std::map<uint32_t,std::vector<TextureRef>> mTextures;
		std::vector<Mesh> mMeshData;
		std::vector<uint8_t> mKeyFrameData;
		std::vector<AnimationClip> mAnimationData;
		std::vector<Matrix4x4>mJointIBMs;
		std::vector<GraphNode> mSceneGraph;

		Skeleton mSkeleton;
		uint32_t mNumJoints = 0;
	protected:

		void Destroy();
	};
}


