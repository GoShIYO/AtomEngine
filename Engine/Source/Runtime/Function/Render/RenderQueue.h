#pragma once
#include "Runtime/Platform/DirectX12/Buffer/UploadBuffer.h"
#include "Runtime/Platform/DirectX12/Buffer/ColorBuffer.h"
#include "Runtime/Platform/DirectX12/Buffer/DepthBuffer.h"
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "Runtime/Core/Math/Frustum.h"
#include "Runtime/Core/Math/Matrix4x4.h"

namespace AtomEngine
{

	class CameraBase;
	struct GlobalConstants;
	struct Mesh;
	struct Joint;

	class RenderQueue
	{
	public:
		enum BatchType { kDefault, kShadows };
		enum DrawType { kZPass, kOpaque, kTransparent, kNumTypes };
		struct DrawInfo
		{
			const CameraBase* m_Camera;
			D3D12_VIEWPORT m_Viewport;
			D3D12_RECT m_Scissor;
		};
		RenderQueue(BatchType type)
		{
			//TODO:typeによって初期化
			mBatchType = type;
			mCamera = nullptr;
			mViewport = {};
			mScissor = {};
			mSortObjects.clear();
			mSortKeys.clear();
			std::memset(mTypeCounts, 0, sizeof(mTypeCounts));
			mCurrentType = kZPass;
			mCurrentDraw = 0;
		}

		void SetCamera(const CameraBase& camera) {mCamera = &camera; }
		void SetViewport(const D3D12_VIEWPORT& viewport) { mViewport = viewport; }
		void SetScissor(const D3D12_RECT& scissor) { mScissor = scissor; }
		void AddRenderTarget(ColorBuffer& RTV)
		{
			mRTV = &RTV;
		}
		void SetDepthStencilTarget(DepthBuffer& DSV) { mDSV = &DSV; }

		const Frustum& GetWorldFrustum() const { return  mCamera->GetWorldSpaceFrustum(); }
		const Frustum& GetViewFrustum() const { return  mCamera->GetViewSpaceFrustum(); }
		const Matrix4x4& GetViewMatrix() const { return  mCamera->GetViewMatrix(); }

		void AddMesh(const Mesh& mesh, float distance,
			D3D12_GPU_VIRTUAL_ADDRESS meshCBV,
			D3D12_GPU_VIRTUAL_ADDRESS materialCBV,
			D3D12_GPU_VIRTUAL_ADDRESS bufferPtr,
			const Joint* skeleton = nullptr);

		void Sort();

		void RenderMeshes(DrawType type, GraphicsContext& context, GlobalConstants& globals);
	private:
		struct SortKey
		{
			union
			{
				uint64_t value;
				struct
				{
					uint64_t objectIdx : 16;
					uint64_t psoIdx : 12;
					uint64_t key : 32;
					uint64_t typeID : 4;
				};
			};
		};

		struct SortObject
		{
			const Mesh* mesh;
			const Joint* skeleton;
			D3D12_GPU_VIRTUAL_ADDRESS meshCBV;
			D3D12_GPU_VIRTUAL_ADDRESS materialCBV;
			D3D12_GPU_VIRTUAL_ADDRESS bufferPtr;
		};

		std::vector<SortObject> mSortObjects;
		std::vector<uint64_t> mSortKeys;
		BatchType mBatchType;
		uint32_t mTypeCounts[kNumTypes];
		DrawType mCurrentType;
		uint32_t mCurrentDraw;

		const CameraBase* mCamera;

		D3D12_VIEWPORT mViewport{};
		D3D12_RECT mScissor{};

		ColorBuffer* mRTV;
		DepthBuffer* mDSV;
	};
}


