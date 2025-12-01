#include "RayTracedShadow.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Platform/DirectX12/Context/ComputeContext.h"

namespace AtomEngine
{
	using namespace Microsoft::WRL;

	void RayTracedShadow::Initialize()
	{
		BuildRootSignature();
		BuildStateObject();
		BuildShaderTables();
	}

	void RayTracedShadow::Render(GraphicsContext& gfxContext, 
		const Camera& camera, 
		const Vector3& lightDir,
		DepthBuffer& depthBuffer)
	{

		RayTracedShadowConstants constants;
		constants.cameraPos = camera.GetPosition();
		constants.invViewProj = camera.GetViewProjMatrix().Inverse().Transpose();
        constants.lightDir = lightDir;

		gfxContext.TransitionResource(mShadowConstants, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		gfxContext.TransitionResource(gRayTracedShadowBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		gfxContext.TransitionResource(depthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		gfxContext.FlushResourceBarriers();

		auto& computeContext = gfxContext.GetComputeContext();
		computeContext.SetRootSignature(mGlobalRootSig);
		computeContext.SetDynamicConstantBufferView(0, sizeof(constants), &constants);
		computeContext.SetDynamicDescriptor(1, 0, mBVH_TLAS.GetSRV());

		ID3D12GraphicsCommandList* pCommandList = gfxContext.GetCommandList();

		ComPtr<ID3D12GraphicsCommandList4> pRaytracingCommandList;
		pCommandList->QueryInterface(IID_PPV_ARGS(&pRaytracingCommandList));


	}

	void RayTracedShadow::BuildRootSignature()
	{
		mGlobalRootSig.Reset(3, 0);
		mGlobalRootSig[0].InitAsConstantBuffer(0);
		mGlobalRootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,0,5);
		mGlobalRootSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,0,1);

        mGlobalRootSig.Finalize(L"RayTracedShadow RootSig", D3D12_ROOT_SIGNATURE_FLAG_NONE);
	}

	void RayTracedShadow::BuildStateObject()
	{

	}

	void RayTracedShadow::BuildShaderTables()
	{

	}
}