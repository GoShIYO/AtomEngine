#pragma once
#include "Mesh.h"
#include "Animation.h"
#include "TextureRef.h"

#include "../Platform/DirectX12/Buffer/GpuBuffer.h"
#include "../Platform/DirectX12/Buffer/UploadBuffer.h"
#include "../Platform/DirectX12/Context/GraphicsContext.h"

#include "../Core/Math/BoundingBox.h"
#include "../Core/Math/BoundingSphere.h"

namespace AtomEngine
{
	enum PSOFlags : uint16_t
	{
		kHasPosition	= 0x001,		// Required
		kHasNormal		= 0x002,		// Required
		kHasTangent		= 0x004,
		kHasUV0			= 0x008,		// Required (for now)
		kHasUV1			= 0x010,
		kAlphaBlend		= 0x020,
		kAlphaTest		= 0x040,
		kTwoSided		= 0x080,
		kHasSkin		= 0x100,		// Implies having indices and weights
	};
	// 描画するグラフノート
	struct GraphNode
	{
		Matrix4x4 xform;
		Quaternion rotation;
		Vector3 scale;

		uint32_t matrixIndex : 28;	// 変換行列要素のインデックス
		uint32_t hasSibling : 1;	// 同級ノードがあるかどうか
		uint32_t hasChildren : 1;	// 子ノードを持つ
		uint32_t staleMatrix : 1;	// 変換行列が古くなっているかどうか
		uint32_t skeletonRoot : 1;  // ボーンのルートかどうか
	};

	struct Joint
	{
		Matrix4x4 posTransform;
		Matrix4x4 nrmTransform;
	};

	class RenderQueue;

	class Model
	{
	public:
		~Model(){Destroy();}

		void Render(RenderQueue& sorter,
			const GpuBuffer& meshConstants,
			const Transform sphereTransforms[],
			const Joint* skeleton) const;

		BoundingSphere m_BoundingSphere;
		AxisAlignedBox m_BoundingBox;
		ByteAddressBuffer mDataBuffer;
		ByteAddressBuffer mMaterialConstants;
		uint32_t mNumNodes;
		uint32_t mNumMeshes;
		uint32_t mNumAnimations;
		uint32_t mNumJoints;
		std::unique_ptr<uint8_t[]>mMeshData;
		std::unique_ptr<GraphNode[]>mSceneGraph;
		std::vector<TextureRef> textures;
		std::unique_ptr<uint8_t[]> mKeyFrameData;
		std::unique_ptr<AnimationCurve[]> mCurveData;
		std::unique_ptr<AnimationSet[]> mAnimations;
		std::unique_ptr<uint16_t[]> mJointIndices;
		std::unique_ptr<Matrix4x4[]> mJointIBMs;

	protected:

		void Destroy();
	};

	class ModelInstance
	{
	public:
		ModelInstance() {}
		~ModelInstance()
		{
			m_MeshConstantsCPU.Destroy();
			m_MeshConstantsGPU.Destroy();
		}
		ModelInstance(std::shared_ptr<const Model> sourceModel);
		ModelInstance(const ModelInstance& modelInstance);

		ModelInstance& operator=(std::shared_ptr<const Model> sourceModel);

		bool IsNull(void) const { return m_Model == nullptr; }

		void Update(GraphicsContext& gfxContext, float deltaTime);
		void Resize(float newRadius);
		Vector3 GetCenter() const;
		float GetRadius() const;
		BoundingSphere GetBoundingSphere() const;
		OrientedBox GetBoundingBox() const;
		void Render(RenderQueue& sorter) const;

		size_t GetNumAnimations(void) const { return m_AnimState.size(); }
		void PlayAnimation(uint32_t animIdx, bool loop);
		void PauseAnimation(uint32_t animIdx);
		void ResetAnimation(uint32_t animIdx);
		void StopAnimation(uint32_t animIdx);
		void UpdateAnimations(float deltaTime);
		void LoopAllAnimations();

		void SetTransform(const Transform& transform) { m_Locator = transform; }
	private:
		std::shared_ptr<const Model> m_Model;
		UploadBuffer m_MeshConstantsCPU;
		ByteAddressBuffer m_MeshConstantsGPU;
		std::unique_ptr<Transform[]> m_BoundingSphereTransforms;
		Transform m_Locator;

		std::unique_ptr<GraphNode[]> m_AnimGraph;   // アニメーションをインスタンス化する際、シーングラフのコピー
		std::vector<AnimationState> m_AnimState;    // アニメーションごと（カーブごとではない）
		std::unique_ptr<Joint[]> m_Skeleton;
	};
}


