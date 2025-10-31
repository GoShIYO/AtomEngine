#pragma once
#include "Runtime/Platform/DirectX12/Buffer/UploadBuffer.h"
#include "Runtime/Platform/DirectX12/Buffer/ColorBuffer.h"
#include "Runtime/Platform/DirectX12/Buffer/DepthBuffer.h"
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "Runtime/Core/Math/Frustum.h"
#include "Runtime/Core/Math/Matrix4x4.h"

struct GlobalConstants;

namespace AtomEngine
{

	class CameraBase;
	struct Mesh;
	struct JointXform;

	struct RenderObject
	{
		const Mesh* mesh = nullptr;
		const JointXform* skeleton = nullptr;

		D3D12_VERTEX_BUFFER_VIEW vbv;
		D3D12_INDEX_BUFFER_VIEW ibv;

		D3D12_GPU_VIRTUAL_ADDRESS meshCBV;
		D3D12_GPU_VIRTUAL_ADDRESS materialCBV;

	};

	class RenderQueue
	{
	public:
		enum BatchType { kDefault, kShadows };
		enum DrawType { kZPass, kOpaque, kTransparent, kNumTypes };
		struct DrawInfo
		{
			const CameraBase* camera = nullptr;
			D3D12_VIEWPORT viewport{};
			D3D12_RECT scissor{};
		};
		RenderQueue(BatchType type)
		{
			//TODO:typeによって初期化
			mBatchType = type;
			mRenderObjects.clear();
			mSortKeys.clear();
			std::memset(mTypeCounts, 0, sizeof(mTypeCounts));
			mCurrentType = kZPass;
			mCurrentDraw = 0;
		}

		void SetDrawInfo(const DrawInfo& info)
		{
			drawInfo = info;
		}

		void AddRenderTarget(ColorBuffer& RTV)
		{
			mRTV = &RTV;
		}
		void SetDepthStencilTarget(DepthBuffer& DSV) { mDSV = &DSV; }

		const Frustum& GetWorldFrustum() const { return  drawInfo.camera->GetWorldSpaceFrustum(); }
		const Frustum& GetViewFrustum() const { return  drawInfo.camera->GetViewSpaceFrustum(); }
		const Matrix4x4& GetViewMatrix() const { return  drawInfo.camera->GetViewMatrix(); }

		void AddMesh(
			const Mesh& mesh,
			const JointXform* skeleton,
			D3D12_VERTEX_BUFFER_VIEW vbv,
			D3D12_INDEX_BUFFER_VIEW ibv,
			D3D12_GPU_VIRTUAL_ADDRESS meshCBV,
			D3D12_GPU_VIRTUAL_ADDRESS materialCBV,
			float distance
		);

		void Sort();

		void RenderMeshes(DrawType type, GraphicsContext& context, GlobalConstants& globals);
	private:

		std::vector<RenderObject> mRenderObjects;
		std::vector<uint64_t> mSortKeys;
		BatchType mBatchType;
		uint32_t mTypeCounts[kNumTypes];
		DrawType mCurrentType;
		uint32_t mCurrentDraw;

		DrawInfo drawInfo;

		ColorBuffer* mRTV = nullptr;
		DepthBuffer* mDSV = nullptr;
	};
}


