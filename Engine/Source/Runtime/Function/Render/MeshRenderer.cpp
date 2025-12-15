#include "MeshRenderer.h"
#include "Runtime/Platform/DirectX12/Buffer/BufferManager.h"
#include "Runtime/Platform/DirectX12/Shader/ShaderCompiler.h"
#include "Runtime/Resource/PrimitiveMesh.h"
#include "Runtime/Core/Math/Math.h"
#include <algorithm>

namespace AtomEngine
{
	struct MeshObject
	{
		const PrimitiveMesh* mesh;
		D3D12_CPU_DESCRIPTOR_HANDLE texture;
		BlendMode blendMode;
		uint64_t key;
	};
	RootSignature sRootSig;
	GraphicsPSO sPSO[BlendMode::kNumBlendModes];

	std::vector<MeshVertex> sVertices;
	std::vector<uint16_t> sIndices;

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

			pso.SetRasterizerState(RasterizerTwoSided);
			pso.SetBlendState(gBlendModeTable[(BlendMode)i]);
			pso.SetDepthStencilState(DepthStateReadOnly);
			pso.SetInputLayout(_countof(layout), layout);
			pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
			pso.SetRenderTargetFormats(1, &gSceneColorBuffer.GetFormat(), gSceneDepthBuffer.GetFormat());
			pso.SetVertexShader(vs.Get());
			pso.SetPixelShader(ps.Get());
			pso.Finalize();
		}

		sVertices = {
			{ Vector3{ -0.5f, -0.5f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f }, Vector2{ 0.0f, 1.0f } },
			{ Vector3{ -0.5f,  0.5f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f }, Vector2{ 0.0f, 0.0f } },
			{ Vector3{  0.5f, -0.5f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f }, Vector2{ 1.0f, 1.0f } },
			{ Vector3{  0.5f,  0.5f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f }, Vector2{ 1.0f, 0.0f } },
		};
        sIndices = {
			0, 1, 2,
			1, 3, 2
		};
	}

	void MeshRenderer::Shutdown()
	{
		sVertices.clear();
        sIndices.clear();
		sMeshObjects.clear();
	}

	void MeshRenderer::AddObject(const PrimitiveMesh* mesh)
	{
		if (!mesh) return;
		MeshObject object;
		object.mesh = mesh;
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

		// 共通頂点/インデックスをセット（全オブジェクト共通の四角形）
		gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gfxContext.SetDynamicVB(0, sVertices.size(), sizeof(MeshVertex), sVertices.data());
		gfxContext.SetDynamicIB(sIndices.size(), sIndices.data());

		// 描画ループ
		BlendMode currentBlend = BlendMode::kBlendNone;
		D3D12_CPU_DESCRIPTOR_HANDLE currentTexture{};

		for (size_t i = 0; i < sMeshObjects.size(); ++i)
		{
			const MeshObject& item = sMeshObjects[i];
			const PrimitiveMesh* mesh = item.mesh;
			if (!mesh) continue;

			//PSO が変わるときだけ切り替え
			if (i == 0 || item.blendMode != currentBlend)
			{
				currentBlend = item.blendMode;
				gfxContext.SetPipelineState(sPSO[static_cast<uint32_t>(currentBlend)]);
			}

			//定数バッファ
			struct
			{
				Matrix4x4 world;
				Matrix4x4 worldInv;
				Matrix4x4 uvTransform;
				Matrix4x4 viewProj;
                Vector4 color;
			}constants;
			constants.world = mesh->GetWorldMatrix();
			constants.worldInv = Math::InverseTranspose(constants.world);
			constants.uvTransform = mesh->GetUVTransformMatrix();
			constants.viewProj = camera->GetViewProjMatrix();
			constants.color = mesh->GetColor();

			// ルートに定数バッファ
			gfxContext.SetDynamicConstantBufferView(0, sizeof(constants), &constants);
			// テクスチャを設定
			gfxContext.SetDynamicDescriptor(1, 0, item.texture);

			// 描画（四角形のインデックス数は sIndices.size()）
			gfxContext.DrawIndexedInstanced((UINT)sIndices.size(), 1, 0, 0, 0);
		}

		// クリア
		sMeshObjects.clear();
	}
}