#include "GameScene.h"
#include "Runtime/Function/Scene/SceneManager.h"
#include "Runtime/Function/Render/RenderSystem.h"
#include "Runtime/Function/Render/Primitive.h"
#include "Runtime/Function/Global/GlobalContext.h"
#include "Runtime/Platform/DirectX12/Core/DirectX12Core.h"
#include "Runtime/Function/Framework/Component/MeshComponent.h"

#include "../Component/ColliderComponent.h"
#include "../Component/VelocityComponent.h"

#include "../Tag.h"

GameScene::GameScene(std::string_view name, SceneManager& manager)
 : Scene(name,manager){
}

bool GameScene::Initialize()
{
	mCamera.SetPosition({ 0.0f, 35.0f, -35.0f });
	mDebugCamera.reset(new DebugCamera(mCamera));
	
	//システム初期化
	InitSystems();

	return Scene::Initialize();
}

void GameScene::Update(float deltaTime)
{
	gContext.imgui->ShowPerformanceWindow(deltaTime);
	//カメラ更新
	mDebugCamera->Update(deltaTime);


	ImGuiHandleObjects();
	DestroyGameObject();
}

void GameScene::AddGameObject(std::unique_ptr<GameObject>&& object)
{
	auto entity = object->GetHandle();
	mGameObjects.insert(std::make_pair(entity, std::move(object)));
}

void GameScene::DestroyGameObject()
{
	auto view = mWorld.View<DeathFlag>();
	for (auto entity : view)
	{
		if (view.get<DeathFlag>(entity).isDead)
		{
			mGameObjects[entity]->Destroy();
			mGameObjects.erase(entity);
		}
	}
}

void GameScene::InitSystems()
{

}

void GameScene::Render()
{
	//Primitive::DrawLine(Vector3(0, 0, 0), Vector3(1, 1, 1), Color::Red, mCamera.GetViewProjMatrix());
	//Primitive::DrawCube(Vector3(0, 0, 0), Vector3(1, 1, 1), testBoxTrans.GetMatrix(), Color::Red, mCamera.GetViewProjMatrix());
	//Primitive::DrawSphere(Vector3(0, 0, 0), 1, Color::Green, mCamera.GetViewProjMatrix(),uint32_t(segment), uint32_t(segment));
	//Primitive::DrawTriangle(Vector3(0, 0, 0), Vector3(1, 1, 1), Vector3(0, 1, 0), Color::Cyan, mCamera.GetViewProjMatrix());
}

void GameScene::Shutdown()
{
	for (auto& object : mGameObjects)
	{
		object.second->Destroy();
	}
}

bool GameScene::Exit()
{
	return false;
}

void GameScene::ImGuiHandleObjects()
{
	ImGui::Begin("Objects");
	int index = 0;
	auto view = mWorld.View<TransformComponent, NameComponent>();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& name = view.get<NameComponent>(entity);
		std::string entityID = " EntityID:" + std::to_string((uint32_t)entity);

		if (ImGui::CollapsingHeader((name.value + entityID).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID(index);

			if (ImGui::TreeNode("Transform"))
			{
				ImGui::DragFloat3("Position##A", transform.transition.ptr(), 0.01f);
				ImGui::DragFloat4("Rotation##A", transform.rotation.ptr(), 0.01f);
				ImGui::DragFloat3("Scale##A", transform.scale.ptr(), 0.01f);
				transform.rotation.Normalize();

				ImGui::TreePop();
			}

			if (auto material = mWorld.TryGet<MaterialComponent>(entity))
			{
				if (ImGui::TreeNode("Materials"))
				{
					for (int matIndex = 0; matIndex < material->mMaterials.size(); matIndex++)
					{
						auto& mat = material->mMaterials[matIndex];
						std::string matLabel = "Material " + std::to_string(matIndex);
						if (ImGui::TreeNode(matLabel.c_str()))
						{
							ImGui::PushID(matIndex);
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
			}
			index++;
			ImGui::PopID();
		}
	}
	ImGui::End();
}
