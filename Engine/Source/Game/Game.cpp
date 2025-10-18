#include "Game.h"
#include <DirectXColors.h>
#include <DirectXMath.h>
#include "imgui.h"
#include "Runtime/Platform/DirectX12/Shader/ConstantBufferStructures.h"
#include "Runtime/Function/Render/RenderQueue.h"

using namespace DirectX;
void Game::Initialize()
{
    m_RootSignature.Reset(1, 0);
    m_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
    m_RootSignature.Finalize(L"box signature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    struct Vertex
    {
        Vector3 Pos;
        Color Color;
    };

    std::array<Vertex, 8> vertices =
    {
        Vertex({ Vector3(-0.5f, -0.5f, -0.5f), Color::White }),
        Vertex({ Vector3(-0.5f, +0.5f, -0.5f), Color::Black }),
        Vertex({ Vector3(+0.5f, +0.5f, -0.5f), Color::Red }),
        Vertex({ Vector3(+0.5f, -0.5f, -0.5f), Color::Green }),
        Vertex({ Vector3(-0.5f, -0.5f, +0.5f), Color::Blue }),
        Vertex({ Vector3(-0.5f, +0.5f, +0.5f), Color::Yellow }),
        Vertex({ Vector3(+0.5f, +0.5f, +0.5f), Color::Cyan }),
        Vertex({ Vector3(+0.5f, -0.5f, +0.5f), Color::Magenta })
    };

    std::array<std::uint16_t, 36> indices =
    {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3,
        4, 3, 7
    };

    m_VertexBuffer.Create(L"vertex buff", 8, sizeof(Vertex), vertices.data());
    m_IndexBuffer.Create(L"index buff", 36, sizeof(std::uint16_t), indices.data());

    D3D12_INPUT_ELEMENT_DESC mInputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
}

void Game::Update(float deltaTime)
{
    static float frameRates[1000] = {};
    static int frameIndex = 0;
    const float frameRate = ImGui::GetIO().Framerate;
    frameRates[frameIndex] = ImGui::GetIO().Framerate;
    frameIndex = (frameIndex + 1) % IM_ARRAYSIZE(frameRates);


    ImGui::Begin("Performance");
    ImGui::Text("FPS: %.2f", frameRate);
    ImGui::Text("DeltaTime: %f Gui: %f", deltaTime, ImGui::GetIO().DeltaTime);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / frameRate);
    ImGui::PlotLines("Frame Rate", frameRates, IM_ARRAYSIZE(frameRates), 0, nullptr, 0.0f, 144.0f, ImVec2(0, 160));
    ImGui::End();

    auto input = Input::GetInstance();
    const GameInputMouseState& mouseState = input->GetMouseState();

    float dx = static_cast<float>(mouseState.positionX);
    float dy = static_cast<float>(mouseState.positionY);

    if (input->IsPressMouse(0) || input->IsPressMouse(1))
    {
        if (input->IsPressMouse(0))
        {
            constexpr float rotateSpeed = 0.005f;

            m_xRotate = dx * rotateSpeed;
            m_yRotate = dy * rotateSpeed;

            m_yRotate = std::clamp(m_yRotate, -Math::HalfPI + 0.1f, Math::HalfPI - 0.1f);
        }
    }

    float x = m_radius * cosf(m_yRotate) * sinf(m_xRotate);
    float y = m_radius * sinf(m_yRotate);
    float z = m_radius * cosf(m_yRotate) * cosf(m_xRotate);

    m_Camera.SetLookAt({ x, y, z }, Vector3::ZERO, Vector3::UP);
    m_Camera.Update();
    m_ViewProjMatrix = m_Camera.GetViewProjMatrix();



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

    gfxContext.TransitionResource(gSceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    gfxContext.ClearColor(gSceneColorBuffer);
    gfxContext.SetViewportAndScissor(m_MainViewport, m_MainScissor);

    gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
    gfxContext.ClearDepth(gSceneDepthBuffer);

    gfxContext.SetRenderTarget(gSceneColorBuffer.GetRTV(), gSceneDepthBuffer.GetDSV());

    gfxContext.SetPipelineState(m_PSO);
    gfxContext.SetRootSignature(m_RootSignature);
    gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    gfxContext.SetVertexBuffer(0, m_VertexBuffer.VertexBufferView());
    gfxContext.SetIndexBuffer(m_IndexBuffer.IndexBufferView());
    gfxContext.SetDynamicConstantBufferView(0, sizeof(m_ViewProjMatrix), &m_ViewProjMatrix);
    gfxContext.DrawIndexedInstanced(36, 1, 0, 0, 0);

    gfxContext.TransitionResource(gSceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ, true);

    gfxContext.Finish();
}

void Game::Shutdown()
{
	m_VertexBuffer.Destroy();
	m_IndexBuffer.Destroy();
}

bool Game::Exit()
{
	return GameApp::Exit();
}
