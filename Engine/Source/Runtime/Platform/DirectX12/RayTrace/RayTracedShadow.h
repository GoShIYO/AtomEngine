#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include "Runtime/Platform/DirectX12/D3dUtility/d3dInclude.h"
#include "Runtime/Platform/DirectX12/Buffer/GpuBuffer.h"
#include "Runtime/Platform/DirectX12/Buffer/ColorBuffer.h"
#include "Runtime/Platform/DirectX12/Buffer/DepthBuffer.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "Runtime/Platform/DirectX12/Context/GraphicsContext.h"
#include "Runtime/Platform/DirectX12/Pipline/PiplineState.h"

namespace AtomEngine
{
	struct RayTracedShadowConstants
	{
		Matrix4x4 invViewProj;
		Vector3 cameraPos;
		float   pad0;
		Vector3 lightDir;
		float   pad1;
	};
	class RayTracedShadow
	{
	public:

		void Initialize();

		void Render(
			GraphicsContext& gfxContext,
			const Camera& camera, 
			const Vector3& lightDir,
			DepthBuffer& depthBuffer);

	private:
		RootSignature mGlobalRootSig;
		Microsoft::WRL::ComPtr<ID3D12StateObject> mDxrStateObject;

		ByteAddressBuffer mRayGenShaderTable;
		ByteAddressBuffer mMissShaderTable;
		ByteAddressBuffer mHitGroupShaderTable;

		ByteAddressBuffer mShadowConstants;

		StructuredBuffer mBVH_TLAS;

	private:
		void BuildRootSignature();
		void BuildStateObject();
		void BuildShaderTables();
	};
}


