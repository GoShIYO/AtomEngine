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

#include "Runtime/Function/Render/Particle/ParticleSystem.h"
#include "Runtime/Function/Render/Particle/ParticleEditor.h"

GameScene::GameScene(std::string_view name, SceneManager& manager)
	: Scene(name, manager)
{
}

bool GameScene::Initialize()
{
	mCamera = &mGameCamera;
	mGameCamera.SetPosition({ 0.0f, 35.0f, -50.0f });
	mDebugCamera.reset(new DebugCamera(mGameCamera));

	const float kWorldScale = 20.0f;

	//システム初期化
	InitSystems();

	ParticleProperty props;
	props.MinStartColor = Color(0x4647A7FF).ToVector4();
	props.MaxStartColor = Color(0x5848F7FF).ToVector4();
	props.MinEndColor = Color(0xCC71EEFF).ToVector4();
	props.MaxEndColor = Color(0xFA8EFAFF).ToVector4();
	
	props.TotalActiveLifetime = 0.0f;
	props.LifeMinMax = float2(3, 5);

	props.Size = Vector4(0.5f, 2, 0.5f, 2);

	props.Velocity = Vector4(-3, 3, -3, 3);

	props.MassMinMax = Vector2(4.5f, 15.0f);
	props.EmitProperties.Gravity = Vector3(0.0f, 0.0f, 0.0f);
	props.EmitProperties.FloorHeight = -0.5f;
	props.EmitProperties.EmitPosW = Vector3(0, 0, 0);
	props.EmitProperties.LastEmitPosW = Vector3(0, 0, 0);
	props.EmitProperties.MaxParticles = 200;
	props.EmitRate = 64;

	props.Spread = float3(40, 20, 80);

	props.TexturePath = L"Asset/Textures/Snow.png";

	//ParticleSystem::CreateParticle(props);

	//ParticleSystem::CreateParticleFromFile("Asset/Particles/bubbles.json");
	//ParticleSystem::CreateParticleFromFile("Asset/Particles/fireworks.json");
	//ParticleSystem::CreateParticleFromFile("Asset/Particles/smoke_ring.json");
	//ParticleSystem::CreateParticleFromFile("Asset/Particles/sparks.json");
	//ParticleSystem::CreateParticleFromFile("Asset/Particles/wavering.json");

	auto model = AssetManager::LoadModel(L"Asset/Models/Stage/stage_test.obj");
	auto obj = mWorld.CreateGameObject("stage");
	obj->AddComponent<MaterialComponent>(model);
	obj->AddComponent<MeshComponent>(model);
	obj->AddComponent<TransformComponent>(Vector3::ZERO, Quaternion::IDENTITY, kWorldScale);
	AddGameObject(std::move(obj));
	
	auto playerModel = AssetManager::LoadModel(L"Asset/Models/human/walk.gltf");
	auto player = mWorld.CreateGameObject("player");
	player->AddComponent<MaterialComponent>(playerModel);
	player->AddComponent<MeshComponent>(playerModel);
	player->AddComponent<TransformComponent>(Vector3(-8, 8, -8), Quaternion::IDENTITY,Vector3(8,8,8));
	player->AddComponent<Body>(Vector3(0.5f, 0.5f, 0.5f), Vector3::ZERO);
	player->AddComponent<PlayerTag>();
	AddGameObject(std::move(player));

	mVoxelWorld.Load("Asset/Voxel/test.vox");

	mTexture = AssetManager::LoadCovertTexture(L"Asset/Textures/uvChecker.png");
	mSprite.reset(new Sprite(mTexture));
	return Scene::Initialize();
}

void GameScene::Update(float deltaTime)
{
	/*static Quaternion q;
	static Radian angle;
	static bool st = false;
	static float t = 0.0f;
	
	ImGui::Begin("Camera");
	if (ImGui::Checkbox("Rotate", &st))
	{
		t = 0.0f;
	}
	if (st && t < 1.0f)
		t += deltaTime * 0.1f;
	angle = Math::Lerp(angle, Radian(Math::TwoPI), t);
	q.FromAngleAxis(angle, mGameCamera.GetForwardVec());
	
	ImGui::DragFloat4("Rotation", q.ptr(), 0.01f);
	q.Normalize();
	ImGui::End();

	mGameCamera.SetRotation(q);*/

	gContext.imgui->ShowPerformanceWindow(deltaTime);
	//カメラ更新
	mDebugCamera->Update(deltaTime);

	mPlayerSystem->Update(mWorld, mGameCamera, deltaTime);

	/*auto& spTrans = mSprite->GetWorldTransform();
	auto& uvTrans = mSprite->GetUVTransform();
	ImGui::Begin("Sprite");
	ImGui::DragFloat2("wPosition", spTrans.transition.ptr(), 0.01f);
	ImGui::DragFloat2("wScale", spTrans.scale.ptr(), 0.01f);
	ImGui::DragFloat3("wRotation", spTrans.rotation.ptr(), 0.01f);

	ImGui::DragFloat2("uPosition", uvTrans.transition.ptr(), 0.01f);
	ImGui::DragFloat2("uScale", uvTrans.scale.ptr(), 0.01f);
	ImGui::DragFloat3("Rotation", uvTrans.rotation.ptr(), 0.01f);
	ImGui::End();*/

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
	mPlayerSystem.reset(new PlayerSystem());
}

void GameScene::Render()
{
	ParticleEditor::Get().Render();
	//mSprite->Render();
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
