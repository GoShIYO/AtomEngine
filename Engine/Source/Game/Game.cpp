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

		texture = AssetManager::LoadCovertTexture(L"Asset/textures/uvChecker.png");
		//texture.CopyGPU(Renderer::GetTextureHeap().GetHeapPointer(), gpuHandle);

		DX12Core::gDevice->CopyDescriptorsSimple(1, gpuHandle, gShadowBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	}

	auto model = AssetManager::LoadModel(L"Asset/Models/suzanne/Suzanne.gltf");
	auto obj = std::make_unique<GameObject>(mWorld.CreateGameObject("DamagedHellmet"));
	obj->AddComponent<MaterialComponent>(model);
	obj->AddComponent<MeshComponent>(model);
	obj->AddComponent<TransformComponent>();
	mGameObjects.push_back(std::move(obj));

}

void Game::Update(float deltaTime)
{
	gContext.imgui->ShowPerformanceWindow(deltaTime);
	m_DebugCamera->Update(deltaTime);

	ImGui::Begin("Objects");
	int index = 0;
	auto view = mWorld.View<TransformComponent, MaterialComponent,NameComponent,MeshComponent>();
	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& material = view.get<MaterialComponent>(entity);
		auto& name = view.get<NameComponent>(entity);
		auto& mesh = view.get<MeshComponent>(entity);

		if (ImGui::CollapsingHeader(name.value.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID(index);

			if (ImGui::TreeNode("Transform"))
			{
				auto& position = transform.GetTranslation();
				auto& rotation = transform.GetRotation();
				auto& scale = transform.GetScale();

				ImGui::DragFloat3("Position##A", position.ptr(), 0.01f);
				ImGui::DragFloat4("Rotation##A", rotation.ptr(), 0.01f);
				ImGui::DragFloat3("Scale##A", scale.ptr(), 0.01f);
				rotation.Normalize();

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Mesh"))
			{
				auto& transform = mesh.GetTransform();

				ImGui::DragFloat3("Position##A", transform.transition.ptr(), 0.01f);
				ImGui::DragFloat4("Rotation##A", transform.rotation.ptr(), 0.01f);
				ImGui::DragFloat3("Scale##A", transform.scale.ptr(), 0.01f);
				transform.rotation.Normalize();

                ImGui::TreePop();
			}

			if (ImGui::TreeNode("Materials"))
			{
				for (int matIndex = 0; matIndex < material.mMaterials.size(); matIndex++)
				{
					auto& mat = material.mMaterials[matIndex];
					std::string matLabel = "Material " + std::to_string(matIndex);
					if (ImGui::TreeNode(matLabel.c_str()))
					{
						ImGui::PushID(matIndex++);

						ImGui::ColorEdit3("Base Color##A", mat.mBaseColor.ptr());
						ImGui::ColorEdit3("Emissive##A", &mat.mEmissive.x);
						ImGui::DragFloat("Metallic##A", &mat.mMetallic, 0.01f, 0.0f, 1.0f);
						ImGui::DragFloat("Roughness##A", &mat.mRoughness, 0.01f, 0.0f, 1.0f);
						ImGui::DragFloat("Normal Scale##A", &mat.mNormalScale, 0.01f, 0.0f, 10.0f);

						ImGui::PopID();
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
			index++;
			ImGui::PopID();
		}
	}
	ImGui::End();
}

void Game::Render()
{
	ImGui::Image((ImTextureID)gpuHandle.GetGpuPtr(), ImVec2(512, 512));

}

void Game::Shutdown()
{
	for (auto& object : mGameObjects)
	{
		object->Destroy();
	}
}

bool Game::Exit()
{
	return GameApp::Exit();
}
