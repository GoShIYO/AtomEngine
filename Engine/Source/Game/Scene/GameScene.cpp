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
	mCamera.SetPosition({ 0.0f, 0.0f, -5.0f });
	mDebugCamera.reset(new DebugCamera(mCamera));

	//test
	{
		mGpuHandle = Renderer::GetTextureHeap().Alloc();

		uvCheckerTex = AssetManager::LoadCovertTexture(L"Asset/textures/uvChecker.png");
		checkBoard = AssetManager::LoadCovertTexture(L"Asset/Models/ExTaskModels/checkerBoard.png");
		//texture.CopyGPU(Renderer::GetTextureHeap().GetHeapPointer(), gpuHandle);
		DX12Core::gDevice->CopyDescriptorsSimple(1, mGpuHandle, gShadowBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	//システム初期化
	InitSystems();

	auto model = AssetManager::LoadModel(L"Asset/Models/suzanne/Suzanne.gltf");
	auto obj = mWorld.CreateGameObject("testObj");
	obj->AddComponent<MaterialComponent>(model);
	obj->AddComponent<MeshComponent>(model);
	obj->AddComponent<TransformComponent>();
	AddGameObject(std::move(obj));

	uvChecker.reset(new Sprite(uvCheckerTex));

	return Scene::Initialize();
}

void GameScene::Update(float deltaTime)
{
	gContext.imgui->ShowPerformanceWindow(deltaTime);
	//カメラ更新
	mDebugCamera->Update(deltaTime);

	mMoveSystem->Update(mWorld, deltaTime);

	ImGuiHandleObjects();
	mCollisionSystem->Update();
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
	mMoveSystem.reset(new MoveSystem);
	mCollisionSystem.reset(new CollisionSystem(mWorld));
}

void GameScene::Render()
{
	uvChecker->Render();
	ImGui::Image((ImTextureID)mGpuHandle.GetGpuPtr(), ImVec2(512, 512));
	ImGui::Begin("Primitive");
	ImGui::DragFloat3("box", &testBoxTrans.transition.x, 0.01f);
	static int segment = 16;
	ImGui::DragInt("Segment", &segment, 1, 3, 128);
	ImGui::End();
	Primitive::DrawLine(Vector3(0, 0, 0), Vector3(1, 1, 1), Color::Red, mCamera.GetViewProjMatrix());
	Primitive::DrawCube(Vector3(0, 0, 0), Vector3(1, 1, 1), testBoxTrans.GetMatrix(), Color::Red, mCamera.GetViewProjMatrix());
	Primitive::DrawSphere(Vector3(0, 0, 0), 1, Color::Green, mCamera.GetViewProjMatrix(),uint32_t(segment), uint32_t(segment));
	Primitive::DrawTriangle(Vector3(0, 0, 0), Vector3(1, 1, 1), Vector3(0, 1, 0), Color::Cyan, mCamera.GetViewProjMatrix());
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

	ImGui::Begin("Sprite");
	auto& worldTrans = uvChecker->GetWorldTransform();
	auto& uvTrans = uvChecker->GetUVTransform();
	ImGui::Separator();
	ImGui::ColorEdit4("Color", spDesc.color.ptr());
	ImGui::DragFloat2("Pivot", &spDesc.pivot.x, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat2("World Position", worldTrans.transition.ptr(), 0.01f);
    ImGui::DragFloat2("World Scale", worldTrans.scale.ptr(), 0.01f);
    ImGui::DragFloat3("World Rotation", worldTrans.rotation.ptr(), 0.01f);
    ImGui::DragFloat2("UV Transform", uvTrans.transition.ptr(), 0.01f);
	ImGui::DragFloat2("UV Scale", uvTrans.scale.ptr(), 0.01f);
    ImGui::DragFloat3("UV Rotation", uvTrans.rotation.ptr(), 0.01f);

	if(ImGui::Button("uvCheker"))
	{
		uvChecker->SetTexture(uvCheckerTex);
	}
	if (ImGui::Button("checkBoard"))
	{
        uvChecker->SetTexture(checkBoard);
	}
	ImGui::End();
	uvChecker->SetDesc(spDesc);
}
