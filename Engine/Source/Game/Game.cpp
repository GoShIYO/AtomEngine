#include "Game.h"
#include "imgui.h"
#include "Runtime/Function/Render/RenderQueue.h"
#include "Runtime/Function/Global/GlobalContext.h"
#include "Runtime/Function/Render/RenderSystem.h"
#include "Runtime/Function/Framework/Component/MeshComponent.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"

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

		DX12Core::gDevice->CopyDescriptorsSimple(1, gpuHandle, gShadowBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	}

	auto model = AssetManager::LoadModel(L"Asset/Models/DamagedHelmet/DamagedHelmet.gltf");
	m_GameObject = std::make_unique<GameObject>(mWorld.CreateGameObject("testObj"));
	m_GameObject->AddComponent<MaterialComponent>(model);
	m_GameObject->AddComponent<MeshComponent>(model);
	m_GameObject->AddComponent<TransformComponent>();

	//auto model2 = AssetManager::LoadModel(L"Asset/Models/Sponza/sponza.gltf");
	//m_GameObject2 = std::make_unique<GameObject>(mWorld.CreateGameObject("testObj2"));
	//m_GameObject2->AddComponent<MaterialComponent>(model2);
	//m_GameObject2->AddComponent<MeshComponent>(model2);
	//m_GameObject2->AddComponent<TransformComponent>();

	testLight.color = Color(1, 0.5f, 0.5f, 1);
	testLight.direction = Vector3(0, -1, 0);
	testLight.innerAngle = Radian(0.0f);
	testLight.outerAngle = Radian(Degree(40.0f));
	testLight.radius = 5.0f;
	testLight.type = LightType::SpotShadow;
	testLight.shadowMatrix = Matrix4x4::IDENTITY;
	testLight.position = Vector3(0, 0, 0);

	lightID = LightManager::AddLight(testLight);
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

	static Vector3 pos2;
	static Quaternion rot2 = Quaternion::IDENTITY;
	static Vector3 scale2 = { 1,1,1 };
	ImGui::DragFloat3("Position2", pos2.ptr(), 0.1f);
	ImGui::DragFloat4("Rotation2", rot2.ptr(), 0.1f);
	ImGui::DragFloat3("Scale2", scale2.ptr(), 0.1f);
	if (m_GameObject2)
	{
		auto& transform2 = m_GameObject2->GetComponent<TransformComponent>();

		transform2.SetTranslation(pos2);
		transform2.SetRotation(rot2);
		transform2.SetScale(scale2);
	}

	ImGui::Begin("Materials");
	auto& material = m_GameObject->GetComponent<MaterialComponent>();
	for (uint32_t i = 0; i < material.mMaterials.size(); i++)
	{
		auto& mat = material.mMaterials[i];
		ImGui::PushID(i);
		ImGui::ColorEdit3("Base Color##A", mat.mBaseColor.ptr());
		ImGui::ColorEdit3("Emissive##A", &mat.mEmissive.x);
		ImGui::DragFloat("Metallic##A", &mat.mMetallic, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Roughness##A", &mat.mRoughness, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("NormalScale##A", &mat.mNormalScale, 0.01f, 0.0f, 10.0f);
		ImGui::PopID();
		ImGui::Separator();
	}
	ImGui::End();


	ImGui::Begin("Lights");
	bool changed = false;
	changed |= ImGui::ColorEdit3("Color", testLight.color.ptr());
	changed |= ImGui::DragFloat3("Direction", testLight.direction.ptr());
	changed |= ImGui::DragFloat("InnerAngle", &testLight.innerAngle, 0.1f, 0.0f);
	changed |= ImGui::DragFloat("OuterAngle", &testLight.outerAngle, 0.1f, 0.0f);
	changed |= ImGui::DragFloat("Radius", &testLight.radius, 0.1f, 0.0f);
	changed |= ImGui::DragFloat3("Position", &testLight.position.x, 0.1f);
	testLight.direction.Normalize();
	ImGui::End();

	if (changed)
	{
		LightManager::UpdateLight(lightID, testLight);
	}
}

void Game::Render()
{
	ImGui::Image((ImTextureID)gpuHandle.GetGpuPtr(), ImVec2(512, 512));

}

void Game::Shutdown()
{
	m_GameObject->Destroy();
	if (m_GameObject2)
		m_GameObject2->Destroy();
}

bool Game::Exit()
{
	return GameApp::Exit();
}
