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
	mCamera.SetPosition({ 0.0f, 0.0f, -5.0f });
	m_DebugCamera.reset(new DebugCamera(mCamera));

	//test
	{
		gpuHandle = Renderer::GetTextureHeap().Alloc();

		test = AssetManager::LoadCovertTexture(L"Asset/textures/uvChecker.png");
		test.CopyGPU(Renderer::GetTextureHeap().GetHeapPointer(), gpuHandle);

	}

	auto model = AssetManager::LoadModel(L"Asset/Models/DamagedHelmet.gltf");
	m_GameObject = std::make_unique<GameObject>(mWorld.CreateGameObject("testObj"));
	m_GameObject->AddComponent<MeshComponent>(model);
	m_GameObject->AddComponent<TransformComponent>();
}

void Game::Update(float deltaTime)
{
	gContext.imgui->ShowPerformanceWindow(deltaTime);

	m_DebugCamera->Update(deltaTime);

	static Vector3 pos;
	static Quaternion rot = Quaternion::IDENTITY;
	static Vector3 scale = { 1,1,1 };
	ImGui::DragFloat3("Position", pos.ptr(), 0.1f);
	ImGui::DragFloat4("Rotation", rot.ptr(), 0.1f);
    ImGui::DragFloat3("Scale", scale.ptr(), 0.1f);
	auto& transform = m_GameObject->GetComponent<TransformComponent>();

    transform.SetTranslation(pos);
    transform.SetRotation(rot);
    transform.SetScale(scale);

}

void Game::Render()
{
	ImGui::Image((ImTextureID)gpuHandle.GetGpuPtr(), ImVec2((float)test.Get()->GetWidth(), (float)test.Get()->GetHeight()));

}

void Game::Shutdown()
{
	m_GameObject->Destroy();
}

bool Game::Exit()
{
	return GameApp::Exit();
}
