#include "Game.h"
#include <DirectXColors.h>
#include <DirectXMath.h>
#include "imgui.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"
#include "Runtime/Function/Render/RenderQueue.h"
#include "Runtime/Function/Global/GlobalContext.h"
#include "Runtime/Function/Render/RenderSystem.h"
#include "Runtime/Function/Framework/Component/MeshComponent.h"

using namespace DirectX;
void Game::Initialize()
{
	m_Camera.SetPosition({ 0.0f, 0.0f, -5.0f });
	m_DebugCamera.reset(new DebugCamera(m_Camera));

	m_RootSignature.Reset(2, 1);
	m_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
	m_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10, D3D12_SHADER_VISIBILITY_PIXEL);

	m_RootSignature.InitStaticSampler(0, SamplerLinearWrapDesc);
	m_RootSignature.Finalize(L"box signature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// struct Vertex
	// {
	// 	Vector3 Pos;
	// 	Color Color;
	// };

	// std::array<Vertex, 8> vertices =
	// {
	// 	Vertex({ Vector3(-0.5f, -0.5f, -0.5f), Color::White }),
	// 	Vertex({ Vector3(-0.5f, +0.5f, -0.5f), Color::Black }),
	// 	Vertex({ Vector3(+0.5f, +0.5f, -0.5f), Color::Red }),
	// 	Vertex({ Vector3(+0.5f, -0.5f, -0.5f), Color::Green }),
	// 	Vertex({ Vector3(-0.5f, -0.5f, +0.5f), Color::Blue }),
	// 	Vertex({ Vector3(-0.5f, +0.5f, +0.5f), Color::Yellow }),
	// 	Vertex({ Vector3(+0.5f, +0.5f, +0.5f), Color::Cyan }),
	// 	Vertex({ Vector3(+0.5f, -0.5f, +0.5f), Color::Magenta })
	// };

	// std::array<std::uint16_t, 36> indices =
	// {
	// 	// front face
	// 	0, 1, 2,
	// 	0, 2, 3,

	// 	// back face
	// 	4, 6, 5,
	// 	4, 7, 6,

	// 	// left face
	// 	4, 5, 1,
	// 	4, 1, 0,

	// 	// right face
	// 	3, 2, 6,
	// 	3, 6, 7,

	// 	// top face
	// 	1, 5, 6,
	// 	1, 6, 2,

	// 	// bottom face
	// 	4, 0, 3,
	// 	4, 3, 7
	// };

	// m_VertexBuffer.Create(L"vertex buff", 8, sizeof(Vertex), vertices.data());
	// m_IndexBuffer.Create(L"index buff", 36, sizeof(std::uint16_t), indices.data());

	D3D12_INPUT_ELEMENT_DESC mInputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TANGENT",  1, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	DXGI_FORMAT ColorFormat = gSceneColorBuffer.GetFormat();
	DXGI_FORMAT DepthFormat = gSceneDepthBuffer.GetFormat();

	auto vsBlob = ShaderCompiler::CompileBlob(L"testShader.hlsl", L"vs_6_2", L"VSMain");
	auto psBlob = ShaderCompiler::CompileBlob(L"testShader.hlsl", L"ps_6_2", L"PSMain");

	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetRasterizerState(RasterizerDefaultCw);
	m_PSO.SetBlendState(BlendDisable);
	m_PSO.SetDepthStencilState(DepthStateReadWrite);
	m_PSO.SetInputLayout(_countof(mInputLayout), mInputLayout);
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PSO.SetRenderTargetFormat(ColorFormat, DepthFormat);
	m_PSO.SetVertexShader(vsBlob.Get());
	m_PSO.SetPixelShader(psBlob.Get());
	m_PSO.Finalize();

	//test
	{
		gpuHandle = Renderer::GetTextureHeap().Alloc();

		test = AssetManager::LoadCovertTexture(L"Asset/textures/uvChecker.png");
		test.CopyGPU(Renderer::GetTextureHeap().GetHeapPointer(), gpuHandle);

	}

	skybox.Initialize();

	auto model = AssetManager::LoadModel(L"Asset/Models/DamagedHelmet.gltf");
	m_GameObject = std::make_unique<GameObject>(world.CreateGameObject("testObj"));
	m_GameObject->AddComponent<MeshComponent>(model);
	m_GameObject->AddComponent<TransformComponent>();

	skybox.SetEnvironmentMap(L"Asset/Textures/EnvironmentMaps/PaperMill/PaperMill_E_3kEnvHDR.dds");
	skybox.SetIBLTextures(
		L"Asset/Textures/EnvironmentMaps/PaperMill/PaperMill_E_3kDiffuseHDR.dds",
		L"Asset/Textures/EnvironmentMaps/PaperMill/PaperMill_E_3kSpecularHDR.dds");
}

void Game::Update(float deltaTime)
{
	gContext.imgui->ShowPerformanceWindow(deltaTime);

	m_DebugCamera->Update(deltaTime);
	m_ViewProjMatrix = m_Camera.GetViewProjMatrix();


	GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Update");
	m_GameObject->GetComponent<MeshComponent>().Update(gfxContext, m_GameObject->GetComponent<TransformComponent>(), deltaTime);
	gfxContext.Finish();

	m_MainViewport.Width = (float)gSceneColorBuffer.GetWidth();
	m_MainViewport.Height = (float)gSceneColorBuffer.GetHeight();
	m_MainViewport.MinDepth = 0.0f;
	m_MainViewport.MaxDepth = 1.0f;

	m_MainScissor.left = 0;
	m_MainScissor.top = 0;
	m_MainScissor.right = (LONG)gSceneColorBuffer.GetWidth();
	m_MainScissor.bottom = (LONG)gSceneColorBuffer.GetHeight();

}

//void Game::Render()
//{
//	ImGui::Image((ImTextureID)gpuHandle.GetGpuPtr(), ImVec2((float)test.Get()->GetWidth(), (float)test.Get()->GetHeight()));
//
//	GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");
//
//	gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
//	gfxContext.ClearDepth(gSceneDepthBuffer);
//
//	gfxContext.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
//	gfxContext.ClearColor(gSceneColorBuffer);
//
//	skybox.Render(gfxContext, m_Camera, m_MainViewport, m_MainScissor);
//
//	gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
//	gfxContext.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
//	gfxContext.SetRenderTarget(gSceneColorBuffer.GetRTV(), gSceneDepthBuffer.GetDSV());
//
//	m_GameObject->GetComponent<MeshComponent>().Render(gfxContext, m_RootSignature, m_PSO, m_ViewProjMatrix);
//
//	gfxContext.Finish();
//}

void Game::Render()
{
	GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");
	const D3D12_VIEWPORT& viewport = m_MainViewport;
	const D3D12_RECT& scissor = m_MainScissor;

	GlobalConstants globals;
	globals.ViewProjMatrix = m_Camera.GetViewProjMatrix();
	globals.SunShadowMatrix = Matrix4x4::IDENTITY;
	globals.CameraPos = m_Camera.GetPosition();
	globals.SunDirection = -Vector3::UP;
	globals.SunIntensity = 1.0f;

	gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	gfxContext.ClearDepth(gSceneDepthBuffer);

	RenderQueue queue(RenderQueue::kDefault);
	queue.SetCamera(m_Camera);
    queue.SetViewportAndScissor(viewport, scissor);
    queue.SetDepthStencilTarget(gSceneDepthBuffer);
	queue.AddRenderTarget(gSceneColorBuffer);

	m_GameObject->GetComponent<MeshComponent>().Render(queue);

	queue.Sort();

	queue.RenderMeshes(RenderQueue::kZPass, gfxContext, globals);
	
	{
		RenderQueue shadowSorter(RenderQueue::kShadows);
		shadowSorter.SetCamera(m_Camera);
		shadowSorter.SetDepthStencilTarget(gShadowBuffer);
	
		m_GameObject->GetComponent<MeshComponent>().Render(shadowSorter);
	
		shadowSorter.Sort();
		shadowSorter.RenderMeshes(RenderQueue::kZPass, gfxContext, globals);
	}

	gfxContext.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	gfxContext.ClearColor(gSceneColorBuffer);
	{
		gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
		gfxContext.SetRenderTarget(gSceneColorBuffer.GetRTV(), gSceneDepthBuffer.GetDSV_DepthReadOnly());
		gfxContext.SetViewportAndScissor(viewport, scissor);

		skybox.Render(gfxContext, m_Camera, viewport, scissor);

		queue.RenderMeshes(RenderQueue::kOpaque, gfxContext, globals);
	}

	queue.RenderMeshes(RenderQueue::kTransparent, gfxContext, globals);

	gfxContext.Finish();

}

//gfxContext.SetRenderTarget(gSceneColorBuffer.GetRTV(), gSceneDepthBuffer.GetDSV_DepthReadOnly());

// gfxContext.SetPipelineState(m_PSO);
// gfxContext.SetRootSignature(m_RootSignature);
// gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
// gfxContext.SetVertexBuffer(0, m_VertexBuffer.VertexBufferView());
// gfxContext.SetIndexBuffer(m_IndexBuffer.IndexBufferView());
// gfxContext.SetDynamicConstantBufferView(0, sizeof(m_ViewProjMatrix), &m_ViewProjMatrix);
// gfxContext.DrawIndexedInstanced(36, 1, 0, 0, 0);

//gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
//gfxContext.SetRenderTarget(gSceneColorBuffer.GetRTV(), gSceneDepthBuffer.GetDSV_DepthReadOnly());
//gfxContext.SetViewportAndScissor(m_MainViewport, m_MainScissor);

void Game::Shutdown()
{
	m_VertexBuffer.Destroy();
	m_IndexBuffer.Destroy();
	m_GameObject->Destroy();
}

bool Game::Exit()
{
	return GameApp::Exit();
}
