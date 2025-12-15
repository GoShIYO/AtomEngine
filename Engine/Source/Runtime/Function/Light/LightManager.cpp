#include "LightManager.h"
#include "Runtime/Platform/DirectX12/Shader/ShaderCompiler.h"
#include "Runtime/Platform/DirectX12/Context/ComputeContext.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"

namespace AtomEngine
{
	std::vector<LightData> LightManager::gLights;

	StructuredBuffer LightManager::gLightBuffer;
	ByteAddressBuffer LightManager::gLightGrid;
	ByteAddressBuffer LightManager::gLightGridBitMask;

	std::unordered_map<uint32_t, uint32_t> LightManager::gLightIDToIndex;
	uint32_t LightManager::gNextLightID = 0;

	enum { kMinLightGridDim = 8 };
	enum { shadowDim = 512 };
	uint32_t LightManager::LightGridDim = 16;

	ColorBuffer LightManager::gLightShadowArray;
	ShadowBuffer LightManager::gLightShadowTempBuffer;
	Matrix4x4 LightManager::gLightShadowMatrix[MAX_LIGHTS];

	RootSignature m_FillLightRootSig;
	ComputePSO m_FillLightGridCS_8(L"Fill Light Grid 8 CS");
	ComputePSO m_FillLightGridCS_16(L"Fill Light Grid 16 CS");
	ComputePSO m_FillLightGridCS_24(L"Fill Light Grid 24 CS");
	ComputePSO m_FillLightGridCS_32(L"Fill Light Grid 32 CS");

	bool LightManager::dirty = true;

	void LightManager::Initialize()
	{
		m_FillLightRootSig.Reset(3, 0);
		m_FillLightRootSig[0].InitAsConstantBuffer(0);
		m_FillLightRootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
		m_FillLightRootSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
		m_FillLightRootSig.Finalize(L"FillLightRS");

		auto fillLightCS_8 = ShaderCompiler::CompileBlob(L"Light/FillLightGridCS_8.hlsl",
			L"cs_6_2", L"CSMain");
		auto fillLightCS_16 = ShaderCompiler::CompileBlob(L"Light/FillLightGridCS_16.hlsl",
			L"cs_6_2", L"CSMain");
		auto fillLightCS_24 = ShaderCompiler::CompileBlob(L"Light/FillLightGridCS_24.hlsl",
			L"cs_6_2", L"CSMain");
		auto fillLightCS_32 = ShaderCompiler::CompileBlob(L"Light/FillLightGridCS_32.hlsl",
			L"cs_6_2", L"CSMain");

		m_FillLightGridCS_8.SetRootSignature(m_FillLightRootSig);
		m_FillLightGridCS_8.SetComputeShader(fillLightCS_8.Get());
		m_FillLightGridCS_8.Finalize();

		m_FillLightGridCS_16.SetRootSignature(m_FillLightRootSig);
		m_FillLightGridCS_16.SetComputeShader(fillLightCS_16.Get());
		m_FillLightGridCS_16.Finalize();

		m_FillLightGridCS_24.SetRootSignature(m_FillLightRootSig);
		m_FillLightGridCS_24.SetComputeShader(fillLightCS_24.Get());
		m_FillLightGridCS_24.Finalize();

		m_FillLightGridCS_32.SetRootSignature(m_FillLightRootSig);
		m_FillLightGridCS_32.SetComputeShader(fillLightCS_32.Get());
		m_FillLightGridCS_32.Finalize();

		// Assumes max resolution of 3840x2160
		uint32_t lightGridCells = DivideByMultiple(3840, kMinLightGridDim) * DivideByMultiple(2160, kMinLightGridDim);
		uint32_t lightGridSizeBytes = lightGridCells * (4 + MAX_LIGHTS * 4);
		gLightGrid.Create(L"LightGrid", lightGridSizeBytes, 1);

		uint32_t lightGridBitMaskSizeBytes = lightGridCells * 4 * 4;
		gLightGridBitMask.Create(L"LightGridBitMask", lightGridBitMaskSizeBytes, 1);

		gLightShadowArray.CreateArray(L"LightShadowArray", shadowDim, shadowDim, MAX_LIGHTS, DXGI_FORMAT_R16_UNORM);
		gLightShadowTempBuffer.Create(L"mightShadowTempBuffer", shadowDim, shadowDim);

		gLightBuffer.Create(L"LightBuffer", MAX_LIGHTS, sizeof(LightData));
		gLights.reserve(MAX_LIGHTS);
	}

	uint32_t LightManager::AddLight(const LightDesc& desc)
	{
		LightData lightData;
		lightData.position = desc.position;
		lightData.radiusSq = desc.radius * desc.radius;
		lightData.color = desc.color.ToVector3();
		lightData.type = static_cast<uint32_t>(desc.type);
		lightData.direction = desc.direction.NormalizedCopy();
		lightData.innerCos = Math::cos(desc.innerAngle);
		lightData.outerCos = Math::cos(desc.outerAngle);
		lightData.shadowMatrix = desc.shadowMatrix;

		if (gLights.size() >= MAX_LIGHTS)
		{
			return 0;
		}
		gLights.push_back(lightData);

		uint32_t id = gNextLightID++;
		gLightIDToIndex[id] = (uint32_t)gLights.size() - 1;
		dirty = true;
		return id;
	}

	uint32_t LightManager::AddPointLight(const Vector3& position, const Color& color, float radius)
	{
		LightData lightData;
		lightData.color = color.ToVector3();
		lightData.position = position;
		lightData.radiusSq = radius * radius;
		lightData.type = (uint32_t)LightType::Point;

		if (gLights.size() >= MAX_LIGHTS)
		{
			return 0;
		}
		gLights.push_back(lightData);
		uint32_t id = gNextLightID++;
		gLightIDToIndex[id] = (uint32_t)gLights.size() - 1;
		dirty = true;
		return id;
	}

	void LightManager::RemoveLight(uint32_t lightID)
	{
		auto it = gLightIDToIndex.find(lightID);
		if (it == gLightIDToIndex.end()) return;

		uint32_t removedIndex = it->second;
		uint32_t lastIndex = (uint32_t)gLights.size() - 1;

		gLights[removedIndex] = gLights[lastIndex];
		gLights.pop_back();

		for (auto& pair : gLightIDToIndex)
		{
			if (pair.second == lastIndex)
			{
				pair.second = removedIndex;
				break;
			}
		}
		gLightIDToIndex.erase(it);
		dirty = true;

	}


	void LightManager::UploadResources()
	{
		if (gLights.empty())return;
		if (dirty)
		{
			for (uint32_t i = 0; i < gLights.size(); ++i)
			{
				const Vector3& pos = gLights[i].position;
				const Vector3& dir = gLights[i].direction;
				float radius = Math::Sqrt(gLights[i].radiusSq);
				float outerCos = gLights[i].outerCos;

				Camera shadowCamera;
				shadowCamera.SetLookAt(pos, pos + dir, Vector3::UP);
				shadowCamera.SetPerspectiveMatrix(outerCos * 2, 1.0f, radius * .05f, radius * 1.0f);
				shadowCamera.Update();

				gLightShadowMatrix[i] = shadowCamera.GetViewProjMatrix();
				Matrix4x4 shadowTextureMatrix = gLightShadowMatrix[i] *
					Matrix4x4(Matrix3x3::MakeScale({ 0.5f, -0.5f, 1.0f }), Vector3(0.5f, 0.5f, 0.0f));
				gLights[i].shadowMatrix = shadowTextureMatrix;
			}

			CommandContext::InitializeBuffer(
				gLightBuffer,
				gLights.data(),
				gLights.size() * (sizeof(LightData))
			);
			dirty = false;
		}
	}

	void LightManager::UpdateLight(uint32_t lightID, const LightDesc& desc)
	{
		auto it = gLightIDToIndex.find(lightID);
		if (it == gLightIDToIndex.end()) return;

		uint32_t idx = it->second;
		LightData& data = gLights[idx];

		data.position = desc.position;
		data.radiusSq = desc.radius * desc.radius;
		data.color = desc.color.ToVector3();
		data.type = static_cast<uint32_t>(desc.type);
		data.direction = desc.direction.NormalizedCopy();
		data.innerCos = std::cos(desc.innerAngle);
		data.outerCos = std::cos(desc.outerAngle);
		dirty = true;
	}

	void LightManager::UpdatePointLight(uint32_t lightID, const Vector3& position, const Color& color, float radius)
	{
		auto it = gLightIDToIndex.find(lightID);
		if (it == gLightIDToIndex.end()) return;

		LightData& data = gLights[it->second];
		data.position = position;
		data.radiusSq = radius * radius;
		data.color = color.ToVector3();
		dirty = true;
	}

	void LightManager::FillLightGrid(GraphicsContext& gfxContext, const Camera& camera)
	{
		ComputeContext& Context = gfxContext.GetComputeContext();

		Context.SetRootSignature(m_FillLightRootSig);

		switch (LightGridDim)
		{
		case  8: Context.SetPipelineState(m_FillLightGridCS_8); break;
		case 16: Context.SetPipelineState(m_FillLightGridCS_16); break;
		case 24: Context.SetPipelineState(m_FillLightGridCS_24); break;
		case 32: Context.SetPipelineState(m_FillLightGridCS_32); break;
		default: ASSERT(false); break;
		}

		//ColorBuffer& LinearDepth = gLinearDepth[DX12Core::GetFrameIndex()];

		Context.TransitionResource(gLightBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		//Context.TransitionResource(LinearDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		Context.TransitionResource(gLightGrid, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(gLightGridBitMask, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		Context.SetDynamicDescriptor(1, 0, gLightBuffer.GetSRV());
		//Context.SetDynamicDescriptor(1, 1, LinearDepth.GetSRV());
		Context.SetDynamicDescriptor(1, 1, gSceneDepthBuffer.GetDepthSRV());
		Context.SetDynamicDescriptor(2, 0, gLightGrid.GetUAV());
		Context.SetDynamicDescriptor(2, 1, gLightGridBitMask.GetUAV());

		uint32_t tileCountX = DivideByMultiple(gSceneColorBuffer.GetWidth(), LightGridDim);
		uint32_t tileCountY = DivideByMultiple(gSceneColorBuffer.GetHeight(), LightGridDim);

		float FarClipDist = camera.GetFarClip();
		float NearClipDist = camera.GetNearClip();

		struct CSConstants
		{
			uint32_t ViewportWidth, ViewportHeight;
			float InvTileDim;
			float NearClip, FarClip;
			uint32_t TileCount;
			Matrix4x4 ViewProjMatrix;
		} csConstants;

		csConstants.ViewportWidth = gSceneColorBuffer.GetWidth();
		csConstants.ViewportHeight = gSceneColorBuffer.GetHeight();
		csConstants.InvTileDim = 1.0f / LightGridDim;
		csConstants.NearClip = NearClipDist;
		csConstants.FarClip = FarClipDist;
		csConstants.TileCount = tileCountX;
		csConstants.ViewProjMatrix = camera.GetViewProjMatrix();
		Context.SetDynamicConstantBufferView(0, sizeof(CSConstants), &csConstants);

		Context.Dispatch(tileCountX, tileCountY, 1);

		Context.TransitionResource(gLightBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		Context.TransitionResource(gLightGrid, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		Context.TransitionResource(gLightGridBitMask, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	void LightManager::Shutdown()
	{
		gLightIDToIndex.clear();

		gLightBuffer.Destroy();
		gLightGrid.Destroy();
		gLightGridBitMask.Destroy();

		gLightShadowArray.Destroy();
		gLightShadowTempBuffer.Destroy();
	}
}

