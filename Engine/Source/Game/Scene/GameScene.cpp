#include "GameScene.h"
#include "imgui.h"
#include "Runtime/Function/Global/GlobalContext.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include "Runtime/Function/Framework/Component/MaterialComponent.h"
#include "Runtime/Function/Framework/Component/MeshComponent.h"
#include "Runtime/Function/Render/Primitive.h"

namespace
{
	const Vector3 kPlayerStartPosition = Vector3(-8, 8, -8);
	Color kLightColor = Color(0.5f, 0.5f, 0.5f);
	Vector3 lightPosition = Vector3::ZERO;
	float lightRadius = 8.0f;
	Vector3 center = 0.0f;
	Vector3 size = 0.0f;
}

GameScene::GameScene(std::string_view name, SceneManager& manager)
	: Scene(name, manager)
{
}

bool GameScene::Initialize()
{
	mCamera = &mGameCamera;
	mGameCamera.SetPosition({ 0.0f, 0.0f, -20.0f });
	mDebugCamera.reset(new DebugCamera(mGameCamera));

	//kLightColor = Color(0.5f, 0.5f, 0.5f);
	//LightManager::AddPointLight(lightPosition, kLightColor, lightRadius);

	auto sponza = AssetManager::LoadModel("Asset/Models/Sponza/Sponza.gltf");
    auto obj = mWorld.CreateGameObject("CornellBox");
	obj->AddComponent<MeshComponent>(sponza);
	obj->AddComponent<MaterialComponent>(sponza);
	obj->AddComponent<TransformComponent>();
	LightManager::CreateRandomLights(sponza->mBoundingBox.GetMin(), sponza->mBoundingBox.GetMax());

	center = (sponza->mBoundingBox.GetMax() + sponza->mBoundingBox.GetMin()) * 0.5f;
	size = sponza->mBoundingBox.GetMax() - sponza->mBoundingBox.GetMin();

	return Scene::Initialize();
}

void GameScene::Update(float deltaTime)
{
#ifndef RELEASE
	gContext.imgui->ShowPerformanceWindow(deltaTime);
#endif

	if (mUseDebugCamera)
	{
		mDebugCamera->Update(deltaTime);
	}
	else
	{
		mGameCamera.Update();
	}

#ifdef _DEBUG
	ImGuiHandleObjects();
#endif
	Primitive::DrawCube(center, size, Vector4(1.0f, 0.0f, 0.0f, 1.0f), mCamera->GetViewProjMatrix());
	DestroyGameObject();
}

void GameScene::Render()
{

}

void GameScene::DestroyGameObject()
{
	auto view = mWorld.View<DeathFlag>();
	view.each([&](auto entity, DeathFlag& flag)
		{
			if (flag.isDead)
			{
				mWorld.DestroyGameObject(entity);
			}
		});
}

void GameScene::Shutdown()
{

}

bool GameScene::Exit()
{
	return false;
}

void GameScene::ImGuiHandleObjects()
{
	ImGui::Begin("Objects");
	int index = 0;
	auto view = mWorld.View<TransformComponent, MaterialComponent, NameComponent>();

	view.each([&](auto entity, TransformComponent& transform, MaterialComponent& material, NameComponent& name)
		{
			std::string entityID = " EntityID:" + std::to_string((uint32_t)entity);

			if (ImGui::CollapsingHeader((name.value + entityID).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushID(index);

				if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::DragFloat3("Position##A", transform.transition.ptr(), 0.01f);
					ImGui::DragFloat4("Rotation##A", transform.rotation.ptr(), 0.01f);
					ImGui::DragFloat3("Scale##A", transform.scale.ptr(), 0.01f);
					transform.rotation.Normalize();

					ImGui::Text("S:%.3f\n", transform.GetScale());
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
				index++;
				ImGui::PopID();
			}
		});
	ImGui::End();

	bool light = false;
	ImGui::Begin("Light");
	light |= ImGui::ColorEdit3("Color", kLightColor.ptr());
	light |= ImGui::DragFloat3("Position", lightPosition.ptr(), 0.01f);
	light |= ImGui::DragFloat("Radius", &lightRadius, 0.01f, 0.1f);
	ImGui::End();
	if (light)
	{
		LightManager::UpdatePointLight(0, lightPosition, kLightColor, lightRadius);
	}

}
