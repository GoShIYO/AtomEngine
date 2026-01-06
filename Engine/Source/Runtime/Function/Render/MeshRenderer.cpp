#include "MeshRenderer.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Platform/DirectX12/Shader/ShaderCompiler.h"
#include "Runtime/Resource/PrimitiveMesh.h"
#include "Runtime/Core/Math/Math.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace AtomEngine
{
	namespace
	{
		struct MeshGeometryData
		{
			std::vector<MeshVertex> vertices;
			std::vector<uint16_t> indices;
		};

		MeshGeometryData CreateQuadGeometry()
		{
			MeshGeometryData data;
			data.vertices = {
				{ Vector3{ -0.5f, -0.5f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f }, Vector2{ 0.0f, 1.0f } },
				{ Vector3{ -0.5f,  0.5f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f }, Vector2{ 0.0f, 0.0f } },
				{ Vector3{  0.5f, -0.5f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f }, Vector2{ 1.0f, 1.0f } },
				{ Vector3{  0.5f,  0.5f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f }, Vector2{ 1.0f, 0.0f } },
			};
			data.indices = {
				0, 1, 2,
				1, 3, 2
			};
			return data;
		}

		MeshGeometryData CreateCubeGeometry()
		{
			MeshGeometryData data;
			data.vertices.reserve(24);
			data.indices.reserve(36);

			const Vector3 normals[6] = {
				{  0.0f,  0.0f,  1.0f },
				{  0.0f,  0.0f, -1.0f },
				{ -1.0f,  0.0f,  0.0f },
				{  1.0f,  0.0f,  0.0f },
				{  0.0f,  1.0f,  0.0f },
				{  0.0f, -1.0f,  0.0f }
			};

			const Vector3 positions[6][4] = {
				{ // +Z
					{ -0.5f, -0.5f,  0.5f },
					{ -0.5f,  0.5f,  0.5f },
					{  0.5f, -0.5f,  0.5f },
					{  0.5f,  0.5f,  0.5f },
				},
				{ // -Z
					{  0.5f, -0.5f, -0.5f },
					{  0.5f,  0.5f, -0.5f },
					{ -0.5f, -0.5f, -0.5f },
					{ -0.5f,  0.5f, -0.5f },
				},
				{ // -X
					{ -0.5f, -0.5f, -0.5f },
					{ -0.5f,  0.5f, -0.5f },
					{ -0.5f, -0.5f,  0.5f },
					{ -0.5f,  0.5f,  0.5f },
				},
				{ // +X
					{ 0.5f, -0.5f,  0.5f },
					{ 0.5f,  0.5f,  0.5f },
					{ 0.5f, -0.5f, -0.5f },
					{ 0.5f,  0.5f, -0.5f },
				},
				{ // +Y
					{ -0.5f, 0.5f,  0.5f },
					{ -0.5f, 0.5f, -0.5f },
					{  0.5f, 0.5f,  0.5f },
					{  0.5f, 0.5f, -0.5f },
				},
				{ // -Y
					{ -0.5f, -0.5f, -0.5f },
					{ -0.5f, -0.5f,  0.5f },
					{  0.5f, -0.5f, -0.5f },
					{  0.5f, -0.5f,  0.5f },
				}
			};

			const Vector2 uvs[4] = {
				{ 0.0f, 1.0f },
				{ 0.0f, 0.0f },
				{ 1.0f, 1.0f },
				{ 1.0f, 0.0f }
			};

			for (size_t face = 0; face < 6; ++face)
			{
				for (size_t i = 0; i < 4; ++i)
				{
					data.vertices.push_back({ positions[face][i], normals[face], uvs[i] });
				}

				const uint16_t baseIndex = static_cast<uint16_t>(face * 4);
				data.indices.push_back(baseIndex + 0);
				data.indices.push_back(baseIndex + 1);
				data.indices.push_back(baseIndex + 2);
				data.indices.push_back(baseIndex + 1);
				data.indices.push_back(baseIndex + 3);
				data.indices.push_back(baseIndex + 2);
			}

			return data;
		}

		MeshGeometryData CreateSphereGeometry(uint32_t latitudeSegments = 16, uint32_t longitudeSegments = 32)
		{
			MeshGeometryData data;
			const float radius = 0.5f;
			const float pi = 3.14159265359f;
			const float twoPi = pi * 2.0f;

			const uint32_t verticalCount = latitudeSegments + 1;
			const uint32_t horizontalCount = longitudeSegments + 1;

			data.vertices.reserve(static_cast<size_t>(verticalCount) * horizontalCount);
			data.indices.reserve(static_cast<size_t>(latitudeSegments) * longitudeSegments * 6);

			for (uint32_t lat = 0; lat <= latitudeSegments; ++lat)
			{
				const float v = static_cast<float>(lat) / static_cast<float>(latitudeSegments);
				const float phi = v * pi;
				const float sinPhi = std::sin(phi);
				const float cosPhi = std::cos(phi);

				for (uint32_t lon = 0; lon <= longitudeSegments; ++lon)
				{
					const float u = static_cast<float>(lon) / static_cast<float>(longitudeSegments);
					const float theta = u * twoPi;
					const float sinTheta = std::sin(theta);
					const float cosTheta = std::cos(theta);

					Vector3 normal{
						sinPhi * cosTheta,
						cosPhi,
						sinPhi * sinTheta
					};

					Vector3 position{
						normal.x * radius,
						normal.y * radius,
						normal.z * radius
					};

					Vector2 uv{ u, 1.0f - v };

					data.vertices.push_back({ position, normal, uv });
				}
			}

			for (uint32_t lat = 0; lat < latitudeSegments; ++lat)
			{
				for (uint32_t lon = 0; lon < longitudeSegments; ++lon)
				{
					const uint16_t current = static_cast<uint16_t>(lat * (longitudeSegments + 1) + lon);
					const uint16_t next = static_cast<uint16_t>(current + longitudeSegments + 1);

					data.indices.push_back(current);
					data.indices.push_back(next);
					data.indices.push_back(current + 1);

					data.indices.push_back(current + 1);
					data.indices.push_back(next);
					data.indices.push_back(next + 1);
				}
			}

			return data;
		}
	}

	struct MeshObject
	{
		const PrimitiveMesh* mesh;
		const MeshGeometryData* geometry;
		D3D12_CPU_DESCRIPTOR_HANDLE texture;
		BlendMode blendMode;
		uint64_t key;
	};

	RootSignature sRootSig;
	GraphicsPSO sPSO[BlendMode::kNumBlendModes];
	std::array<MeshGeometryData, static_cast<size_t>(PrimitiveMeshType::Count)> sGeometryCache;
	std::vector<MeshObject> sMeshObjects;

	void MeshRenderer::Initialize()
	{
		SamplerDesc SamplerDesc;

		sRootSig.Reset(2, 1);
		sRootSig.InitStaticSampler(0, SamplerLinearClampDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		sRootSig[0].InitAsConstantBuffer(0);
		sRootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
		sRootSig.Finalize(L"MeshRenderer rootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		D3D12_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			 D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			 D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			  D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		auto vs = ShaderCompiler::CompileBlob(L"MeshVS.hlsl", L"vs_6_2");
		auto ps = ShaderCompiler::CompileBlob(L"MeshPS.hlsl", L"ps_6_2");

		for (uint32_t i = 0; i < kNumBlendModes; i++)
		{
			auto& pso = sPSO[i];

			pso.SetRootSignature(sRootSig);

			pso.SetRasterizerState(RasterizerDefault);
			pso.SetBlendState(gBlendModeTable[(BlendMode)i]);
			pso.SetDepthStencilState(DepthStateReadOnly);
			pso.SetInputLayout(_countof(layout), layout);
			pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
			pso.SetRenderTargetFormats(1, &gSceneColorBuffer.GetFormat(), gSceneDepthBuffer.GetFormat());
			pso.SetVertexShader(vs.Get());
			pso.SetPixelShader(ps.Get());
			pso.Finalize();
		}

		sGeometryCache.fill({});
		sGeometryCache[static_cast<size_t>(PrimitiveMeshType::Quad)] = CreateQuadGeometry();
		sGeometryCache[static_cast<size_t>(PrimitiveMeshType::Cube)] = CreateCubeGeometry();
		sGeometryCache[static_cast<size_t>(PrimitiveMeshType::Sphere)] = CreateSphereGeometry();
	}

	void MeshRenderer::Shutdown()
	{
		for (auto& geometry : sGeometryCache)
		{
			geometry.vertices.clear();
			geometry.indices.clear();
		}
		sMeshObjects.clear();
	}

	void MeshRenderer::AddObject(const PrimitiveMesh* mesh)
	{
		if (!mesh) return;

		const size_t geometryIndex = static_cast<size_t>(mesh->GetPrimitiveType());
		if (geometryIndex >= sGeometryCache.size())
			return;

		const MeshGeometryData* geometry = &sGeometryCache[geometryIndex];
		if (geometry->vertices.empty() || geometry->indices.empty())
			return;

		MeshObject object;
		object.mesh = mesh;
		object.geometry = geometry;
		object.blendMode = mesh->GetBlendMode();
		object.texture = mesh->GetTexture();
		object.key = mesh->GetSortKey();

		sMeshObjects.push_back(object);
	}

	void MeshRenderer::Sort()
	{
		std::sort(sMeshObjects.begin(), sMeshObjects.end(),
			[](const MeshObject& a, const MeshObject& b) -> bool
			{
				return a.key < b.key;
			});
	}

	void MeshRenderer::Render(GraphicsContext& gfxContext, const Camera* camera)
	{
		if (sMeshObjects.empty())
			return;

		Sort();

		gfxContext.SetRootSignature(sRootSig);
		gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		BlendMode currentBlend = BlendMode::kBlendNone;
		const MeshGeometryData* currentGeometry = nullptr;

		for (size_t i = 0; i < sMeshObjects.size(); ++i)
		{
			const MeshObject& item = sMeshObjects[i];
			const PrimitiveMesh* mesh = item.mesh;
			if (!mesh || !item.geometry) continue;

			if (i == 0 || item.blendMode != currentBlend)
			{
				currentBlend = item.blendMode;
				gfxContext.SetPipelineState(sPSO[static_cast<uint32_t>(currentBlend)]);
			}

			if (item.geometry != currentGeometry)
			{
				currentGeometry = item.geometry;
				gfxContext.SetDynamicVB(0, currentGeometry->vertices.size(), sizeof(MeshVertex), currentGeometry->vertices.data());
				gfxContext.SetDynamicIB(currentGeometry->indices.size(), currentGeometry->indices.data());
			}

			struct
			{
				Matrix4x4 world;
				Matrix4x4 worldInv;
				Matrix4x4 uvTransform;
				Matrix4x4 viewProj;
				Vector4 color;
			} constants;

			constants.world = mesh->GetWorldMatrix();
			constants.worldInv = Math::InverseTranspose(constants.world);
			constants.uvTransform = mesh->GetUVTransformMatrix();
			constants.viewProj = camera->GetViewProjMatrix();
			constants.color = mesh->GetColor();

			// ルートに定数バッファ
			gfxContext.SetDynamicConstantBufferView(0, sizeof(constants), &constants);
			// テクスチャを設定
			gfxContext.SetDynamicDescriptor(1, 0, item.texture);

			// 描画indices
			gfxContext.DrawIndexedInstanced(static_cast<UINT>(currentGeometry->indices.size()), 1, 0, 0, 0);
		}

		sMeshObjects.clear();
	}
}