#include "Game.h"
#include <DirectXColors.h>
#include <DirectXMath.h>
#include "imgui.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"
#include "Runtime/Function/Render/RenderQueue.h"

using namespace DirectX;
//void Game::Initialize()
//{
//    m_RootSignature.Reset(1, 0);
//    m_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
//    m_RootSignature.Finalize(L"box signature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//
//    struct Vertex
//    {
//        XMFLOAT3 Pos;
//        XMFLOAT4 Color;
//    };
//
//    std::array<Vertex, 8> vertices =
//    {
//        Vertex({ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::White) }),
//        Vertex({ XMFLOAT3(-0.5f, +0.5f, -0.5f), XMFLOAT4(Colors::Black) }),
//        Vertex({ XMFLOAT3(+0.5f, +0.5f, -0.5f), XMFLOAT4(Colors::Red) }),
//        Vertex({ XMFLOAT3(+0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::Green) }),
//        Vertex({ XMFLOAT3(-0.5f, -0.5f, +0.5f), XMFLOAT4(Colors::Blue) }),
//        Vertex({ XMFLOAT3(-0.5f, +0.5f, +0.5f), XMFLOAT4(Colors::Yellow) }),
//        Vertex({ XMFLOAT3(+0.5f, +0.5f, +0.5f), XMFLOAT4(Colors::Cyan) }),
//        Vertex({ XMFLOAT3(+0.5f, -0.5f, +0.5f), XMFLOAT4(Colors::Magenta) })
//    };
//
//    std::array<std::uint16_t, 36> indices =
//    {
//        // front face
//        0, 1, 2,
//        0, 2, 3,
//
//        // back face
//        4, 6, 5,
//        4, 7, 6,
//
//        // left face
//        4, 5, 1,
//        4, 1, 0,
//
//        // right face
//        3, 2, 6,
//        3, 6, 7,
//
//        // top face
//        1, 5, 6,
//        1, 6, 2,
//
//        // bottom face
//        4, 0, 3,
//        4, 3, 7
//    };
//
//    m_VertexBuffer.Create(L"vertex buff", 8, sizeof(Vertex), vertices.data());
//    m_IndexBuffer.Create(L"index buff", 36, sizeof(std::uint16_t), indices.data());
//
//    D3D12_INPUT_ELEMENT_DESC mInputLayout[] =
//    {
//        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
//    };
//
//
//    DXGI_FORMAT ColorFormat = gSceneColorBuffer.GetFormat();
//    DXGI_FORMAT DepthFormat = gSceneDepthBuffer.GetFormat();
//
//    auto vsBlob = ShaderCompiler::CompileBlob(L"testShader.hlsl", L"vs_6_2", L"VSMain");
//    auto psBlob = ShaderCompiler::CompileBlob(L"testShader.hlsl", L"ps_6_2", L"PSMain");
//
//    m_PSO.SetRootSignature(m_RootSignature);
//    m_PSO.SetRasterizerState(RasterizerDefault);
//    m_PSO.SetBlendState(BlendDisable);
//    m_PSO.SetDepthStencilState(DepthStateReadOnly);
//    m_PSO.SetInputLayout(_countof(mInputLayout), mInputLayout);
//    m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
//    m_PSO.SetRenderTargetFormat(ColorFormat, DepthFormat);
//    m_PSO.SetVertexShader(vsBlob.Get());
//    m_PSO.SetPixelShader(psBlob.Get());
//    m_PSO.Finalize();
//}
//
//void Game::Update(float deltaTime)
//{
//    auto input = Input::GetInstance();
//    const GameInputMouseState& mouseState = input->GetMouseState();
//    
//    if (input->IsPressMouse(0) || input->IsPressMouse(1))
//    {
//        float dx = static_cast<float>(mouseState.positionX);
//        float dy = static_cast<float>(mouseState.positionY);
//
//        if (input->IsPressMouse(0))
//        {
//            m_xRotate += (dx - m_xDiff);
//            m_yRotate += (dy - m_yDiff);
//            m_yRotate = (std::max)(-Math::HalfPI + 0.1f, m_yRotate);
//            m_yRotate = (std::min)(Math::HalfPI - 0.1f, m_yRotate);
//        }
//        else
//        {
//            m_radius += dx - dy - (m_xDiff - m_yDiff);
//        }
//
//        m_xDiff = dx;
//        m_yDiff = dy;
//
//        m_xLast += dx;
//        m_yLast += dy;
//    }
//    else
//    {
//        m_xDiff = 0.0f;
//        m_yDiff = 0.0f;
//        m_xLast = 0.0f;
//        m_yLast = 0.0f;
//    }
//
//    float x = m_radius * cosf(m_yRotate) * sinf(m_xRotate);
//    float y = m_radius * sinf(m_yRotate);
//    float z = m_radius * cosf(m_yRotate) * cosf(m_xRotate);
//
//    m_Camera.SetEyeAtUp({ x, y, z }, Vector3::ZERO, Vector3::UP);
//    m_Camera.Update();
//
//    m_ViewProjMatrix = m_Camera.GetViewProjMatrix();
//
//    m_MainViewport.Width = (float)gSceneColorBuffer.GetWidth();
//    m_MainViewport.Height = (float)gSceneColorBuffer.GetHeight();
//    m_MainViewport.MinDepth = 0.0f;
//    m_MainViewport.MaxDepth = 1.0f;
//
//    m_MainScissor.left = 0;
//    m_MainScissor.top = 0;
//    m_MainScissor.right = (LONG)gSceneColorBuffer.GetWidth();
//    m_MainScissor.bottom = (LONG)gSceneColorBuffer.GetHeight();
//}
//
//void Game::Render()
//{
//    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");
//
//    gfxContext.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
//    gfxContext.ClearColor(gSceneColorBuffer);
//    gfxContext.SetViewportAndScissor(m_MainViewport, m_MainScissor);
//
//    gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
//    gfxContext.ClearDepth(gSceneDepthBuffer);
//
//    gfxContext.SetRenderTarget(gSceneColorBuffer.GetRTV(), gSceneDepthBuffer.GetDSV_DepthReadOnly());
//
//    gfxContext.SetPipelineState(m_PSO);
//    gfxContext.SetRootSignature(m_RootSignature);
//    gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//    gfxContext.SetVertexBuffer(0, m_VertexBuffer.VertexBufferView());
//    gfxContext.SetIndexBuffer(m_IndexBuffer.IndexBufferView());
//    gfxContext.SetDynamicConstantBufferView(0, sizeof(m_ViewProjMatrix), &m_ViewProjMatrix);
//    gfxContext.DrawIndexedInstanced(36, 1, 0, 0, 0);
//
//    gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ, true);
//
//    gfxContext.Finish();
//}

void Game::Shutdown()
{
	m_Model = nullptr;
	m_VertexBuffer.Destroy();
	m_IndexBuffer.Destroy();
}

bool Game::Exit()
{
	return GameApp::Exit();
}

void Game::Initialize()
{
	m_Model = AssetManager::LoadModel(L"Asset/Models/DamagedHelmet.gltf");
}

void Game::Update(float deltaTime)
{

	m_Camera.Update();

	GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Update");

	m_Model.Update(gfxContext, deltaTime);

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

void Game::Render()
{
	GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");

	const D3D12_VIEWPORT& viewport = m_MainViewport;
	const D3D12_RECT& scissor = m_MainScissor;

	{
		// Update global constants
		float costheta = cosf(g_SunOrientation);
		float sintheta = sinf(g_SunOrientation);
		float cosphi = cosf(g_SunInclination * 3.14159f * 0.5f);
		float sinphi = sinf(g_SunInclination * 3.14159f * 0.5f);

		Vector3 SunDirection = Math::Normalize(Vector3(costheta * cosphi, sinphi, sintheta * cosphi));
		//Vector3 ShadowBounds = Vector3(m_Model.GetRadius());
		////m_SunShadowCamera.UpdateMatrix(-SunDirection, m_ModelInst.GetCenter(), ShadowBounds,
		//m_SunShadowCamera.UpdateMatrix(-SunDirection, Vector3(0, -500.0f, 0), Vector3(5000, 3000, 3000),
		//	(uint32_t)g_ShadowBuffer.GetWidth(), (uint32_t)g_ShadowBuffer.GetHeight(), 16);

		GlobalConstants globals;
		globals.ViewProjMatrix = m_Camera.GetViewProjMatrix();
		globals.SunShadowMatrix = Matrix4x4::IDENTITY;
		globals.CameraPos = m_Camera.GetPosition();
		globals.SunDirection = SunDirection;
		globals.SunIntensity = Vector3(g_SunLightIntensity);

		// Begin rendering depth
		gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
		gfxContext.ClearDepth(gSceneDepthBuffer);

		RenderQueue sorter(RenderQueue::kDefault);
		sorter.SetCamera(m_Camera);
		sorter.SetViewport(viewport);
		sorter.SetScissor(scissor);
		sorter.SetDepthStencilTarget(gSceneDepthBuffer);
		sorter.AddRenderTarget(gSceneColorBuffer);

		m_Model.Render(sorter);

		sorter.Sort();

		{
			sorter.RenderMeshes(RenderQueue::kZPass, gfxContext, globals);
		}

		gfxContext.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

		gfxContext.ClearColor(gSceneColorBuffer);

		{
			gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
			gfxContext.SetRenderTarget(gSceneColorBuffer.GetRTV(), gSceneDepthBuffer.GetDSV_DepthReadOnly());
			gfxContext.SetViewportAndScissor(viewport, scissor);

			sorter.RenderMeshes(RenderQueue::kOpaque, gfxContext, globals);
		}

		sorter.RenderMeshes(RenderQueue::kTransparent, gfxContext, globals);
	}

	gfxContext.Finish();
}
