#include "GameScene.h"
#include "Runtime/Function/Scene/SceneManager.h"
#include "Runtime/Function/Render/RenderSystem.h"
#include "Runtime/Function/Render/Primitive.h"
#include "Runtime/Function/Global/GlobalContext.h"
#include "Runtime/Function/Framework/Component/MeshComponent.h"
#include "Runtime/Function/Light/LightManager.h"

#include "../Component/VelocityComponent.h"
#include "../Component/ItemComponent.h"
#include "../Component/GoalComponent.h"
#include "../Component/VoxelColliderComponent.h"
#include "../Component/PlatformComponent.h"
#include "../System/ItemSystem.h"
#include "../System/GoalSystem.h"
#include "../System/SoundManaged.h"
#include "../Editor/ItemGoalEditor.h"
#include "../Tag.h"
#include "../Voxel/VoxelWorld.h"

class ClearScene;
extern std::unique_ptr<AtomEngine::Scene> CreateClearScene(const std::string& name, AtomEngine::SceneManager& manager);

extern void SetClearSceneGameResult(AtomEngine::Scene* scene, int collected, int total, float time);

namespace
{
	const Vector3 kPlayerStartPosition = Vector3(-8, 8, -8);
}

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

	// ゲーム状態初期化
	mGameTime = 0.0f;
	mIsGameClear = false;
	mClearDelayTimer = 0.0f;

	// システム初期化
	InitSystems();

	// ステージ
	auto model = AssetManager::LoadModel(L"Asset/Models/Stage/stage.obj");
	auto obj = mWorld.CreateGameObject("stage");
	obj->AddComponent<MaterialComponent>(model);
	obj->AddComponent<MeshComponent>(model);
	obj->AddComponent<TransformComponent>(Vector3::ZERO, Quaternion::IDENTITY, kWorldScale);
	AddGameObject(std::move(obj));

	// プレイヤー
	auto playerModel = AssetManager::LoadModel(L"Asset/Models/panda/panda.gltf");
	auto player = mWorld.CreateGameObject("player");
	player->AddComponent<MaterialComponent>(playerModel);
	player->AddComponent<MeshComponent>(playerModel);
	player->AddComponent<TransformComponent>(kPlayerStartPosition, Quaternion::IDENTITY, Vector3(8, 8, 8));
	player->AddComponent<VelocityComponent>();
	player->AddComponent<PlayerTag>();
	VoxelColliderComponent playerCollider;
	playerCollider.halfExtents = Vector3(2.0f, 2.0f, 2.5f);
	playerCollider.offset = Vector3(0.0f, 2.0f, 0.0f);
	playerCollider.enableCollision = true;
	playerCollider.enableSliding = true;
	playerCollider.groundCheckDistance = 0.1f;
	player->AddComponent<VoxelColliderComponent>(playerCollider);
	mPlayerEntity = player->GetHandle();
	AddGameObject(std::move(player));

	// Voxel World
	VoxelWorldComponent voxelComp{};
	voxelComp.world.voxelSize = 2;
	voxelComp.world.Load("Asset/Voxel/stage.vox");

	auto voxel = mWorld.CreateGameObject("voxel_world");
	voxel->AddComponent<VoxelWorldComponent>(std::move(voxelComp));
	mVoxelWorldEntity = voxel->GetHandle();
	AddGameObject(std::move(voxel));

	if (mWorld.HasComponent<VoxelWorldComponent>(mVoxelWorldEntity))
	{
		auto& vw = mWorld.GetComponent<VoxelWorldComponent>(mVoxelWorldEntity);
		mVoxelCollisionSystem->SetVoxelWorld(&vw.world);
	}

	// 各システムの初期化（JSONから読み込み）
	mPlatformSystem->Initialize(mWorld, "Asset/Config/platforms.json");
	mLadderSystem->Initialize(mWorld, "Asset/Config/ladders.json");
	mItemSystem->Initialize(mWorld, "Asset/Config/items.json");
	mGoalSystem->Initialize(mWorld, "Asset/Config/goals.json");

	// カメラ設定
	mInGameCameraController.reset(new InGameCameraController(mGameCamera));
	mInGameCameraController->SetWorld(&mWorld);
	mInGameCameraController->SetTargetEntity(mPlayerEntity);
	mInGameCameraController->SetDistance(60.0f);
	mInGameCameraController->SetDistanceRange(60.0f, 120.0f);
	mInGameCameraController->SetHeight(5.0f);
	mInGameCameraController->SetPitch(0.5f);
	mInGameCameraController->SetPitchRange(0.0f, 1.2f);

	Audio::GetInstance()->Play(Sound::gSoundMap["bgm"], true);
	Audio::GetInstance()->SetVolume(Sound::gSoundMap["bgm"], 0.5f);

	return Scene::Initialize();
}

void GameScene::Update(float deltaTime)
{
#ifndef RELEASE
	gContext.imgui->ShowPerformanceWindow(deltaTime);
#endif
	// クリア後の遷移処理
	if (mIsGameClear)
	{
		mClearDelayTimer += deltaTime;
		if (mClearDelayTimer > 2.0f)
		{
			OnGameClear();
			return;
		}
	}
	else
	{
		// ゲーム時間更新
		mGameTime += deltaTime;
	}

	// カメラ更新
	if (mUseDebugCamera)
	{
		mDebugCamera->Update(deltaTime);
	}
	else
	{
		mInGameCameraController->Update(deltaTime);
	}

	mPlayerSystem->Update(mWorld, mGameCamera, deltaTime);

	// システム更新
	mPlatformSystem->Update(mWorld, deltaTime);
	mLadderSystem->Update(mWorld, deltaTime);
	mItemSystem->Update(mWorld, deltaTime);
	mGoalSystem->Update(mWorld, deltaTime);
	mVoxelCollisionSystem->Update(mWorld, deltaTime);

	mMoveSystem->Update(mWorld, deltaTime);

	// ゴール判定
	if (!mIsGameClear && mGoalSystem->IsGoalReached())
	{
		mIsGameClear = true;
		mClearDelayTimer = 0.0f;
	}

#ifdef _DEBUG
	ImGuiHandleObjects();
#endif

	DestroyGameObject();
}

void GameScene::Render()
{
#ifndef RELEASE
	mLadderEditor->RenderUI(mWorld, mGameCamera);
	mLadderEditor->RenderVisualization(mWorld, mGameCamera.GetViewProjMatrix());

	if (mShowPlatformEditor)
	{
		mPlatformEditor->RenderVisualization(mWorld, mCamera->GetViewProjMatrix());
	}

	if (mShowItemGoalEditor)
	{
		mItemGoalEditor->RenderVisualization(mWorld, mCamera->GetViewProjMatrix());
	}
#endif
	// ゲームUI描画
	RenderGameUI();
}

void GameScene::RenderGameUI()
{
	// アイテム数表示
	ImGuiWindowFlags uiFlags = ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav;

	// 左上にアイテム数
	ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.6f);

	if (ImGui::Begin("ItemCount", nullptr, uiFlags))
	{
		ImGui::SetWindowFontScale(1.5f);

		int collected = mItemSystem->GetCollectedCount();
		int total = mItemSystem->GetTotalCount();

		ImVec4 itemColor = (collected == total && total > 0) ?
			ImVec4(1.0f, 0.84f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

		ImGui::TextColored(itemColor, "Items: %d / %d", collected, total);

		ImGui::SetWindowFontScale(1.0f);
	}
	ImGui::End();

	// 右上に時間
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(viewport->Size.x - 175, 20), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.6f);

	if (ImGui::Begin("GameTime", nullptr, uiFlags))
	{
		ImGui::SetWindowFontScale(1.5f);

		int minutes = static_cast<int>(mGameTime) / 60;
		int seconds = static_cast<int>(mGameTime) % 60;
		int milliseconds = static_cast<int>((mGameTime - static_cast<int>(mGameTime)) * 100);

		ImGui::Text("%02d:%02d.%02d", minutes, seconds, milliseconds);

		ImGui::SetWindowFontScale(1.0f);
	}
	ImGui::End();

	// クリア時のメッセージ
	if (mIsGameClear)
	{
		ImVec2 center = viewport->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowBgAlpha(0.8f);

		if (ImGui::Begin("ClearMessage", nullptr, uiFlags))
		{
			ImGui::SetWindowFontScale(3.0f);

			// 点滅効果
			float alpha = (std::sin(mClearDelayTimer * 5.0f) + 1.0f) * 0.5f * 0.5f + 0.5f;
			ImVec4 clearColor = ImVec4(1.0f, 0.84f, 0.0f, alpha);

			ImGui::TextColored(clearColor, "GOAL!");

			ImGui::SetWindowFontScale(1.0f);
		}
		ImGui::End();
	}
}

void GameScene::OnGameClear()
{
	auto clearScene = CreateClearScene("Clear", mManager);

	SetClearSceneGameResult(clearScene.get(),
		mItemSystem->GetCollectedCount(),
		mItemSystem->GetTotalCount(),
		mGameTime);

	RequestReplaceScene(std::move(clearScene));
}

void GameScene::AddGameObject(std::unique_ptr<GameObject>&& object)
{
	auto entity = object->GetHandle();
	mGameObjects.insert(std::make_pair(entity, std::move(object)));
}

void GameScene::DestroyGameObject()
{
	auto view = mWorld.View<DeathFlag>();
	view.each([&](auto entity, DeathFlag& flag)
		{
			if (flag.isDead)
			{
				mGameObjects[entity]->Destroy();
				mGameObjects.erase(entity);
			}
		});
}

void GameScene::InitSystems()
{
	mPlayerSystem.reset(new PlayerSystem());
	mMoveSystem.reset(new MoveSystem());
	mVoxelCollisionSystem.reset(new VoxelCollisionSystem());

	mPlatformSystem.reset(new PlatformSystem());
	mPlatformEditor.reset(new PlatformEditor());
	mPlatformEditor->SetPlatformSystem(mPlatformSystem.get());

	mLadderSystem.reset(new LadderSystem());
	mLadderEditor.reset(new LadderEditor());
	mLadderEditor->SetLadderSystem(mLadderSystem.get());

	mItemSystem.reset(new ItemSystem());
	mItemSystem->SetAddGameObjectCallback([this](std::unique_ptr<GameObject>&& obj)
		{
			AddGameObject(std::move(obj));
		});

	mGoalSystem.reset(new GoalSystem());

	mItemGoalEditor.reset(new ItemGoalEditor());
	mItemGoalEditor->SetItemSystem(mItemSystem.get());
	mItemGoalEditor->SetGoalSystem(mGoalSystem.get());
}

void GameScene::CreateTestPlatforms()
{
	{
		PlatformCreateInfo info;
		info.position = Vector3(0.0f, 5.0f, 0.0f);
		info.size = Vector3(6.0f, 1.0f, 6.0f);
		info.motionType = PlatformMotionType::Linear;
		info.voxelSize = 2.0f;
		info.modelPath = L"Asset/Models/Cube/cube.obj";

		Entity platformEntity = mPlatformSystem->CreatePlatform(mWorld, info);

		if (mWorld.HasComponent<PlatformComponent>(platformEntity))
		{
			auto& platform = mWorld.GetComponent<PlatformComponent>(platformEntity);
			platform.startPosition = Vector3(0.0f, 5.0f, 0.0f);
			platform.endPosition = Vector3(20.0f, 5.0f, 0.0f);
			platform.moveSpeed = 5.0f;
			platform.pingPong = true;
		}
	}
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

	view.each([&](auto entity, TransformComponent& transform, NameComponent& name)
		{
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
		});
	ImGui::End();

	if (mShowPlatformEditor)
	{
		mPlatformEditor->RenderUI(mWorld, mGameCamera);
	}

	if (mShowItemGoalEditor)
	{
		mItemGoalEditor->RenderUI(mWorld, mGameCamera);
	}

	ImGui::Begin("Editor Controls");
	ImGui::Checkbox("Show Platform Editor", &mShowPlatformEditor);
	ImGui::Checkbox("Show Item/Goal Editor", &mShowItemGoalEditor);
	ImGui::Checkbox("Use Debug Camera", &mUseDebugCamera);

	if (!mUseDebugCamera && mInGameCameraController)
	{
		ImGui::Separator();
		ImGui::Text("Camera Settings");

		float distance = mInGameCameraController->GetDistance();
		if (ImGui::SliderFloat("Distance", &distance, 60.0f, 120.0f))
		{
			mInGameCameraController->SetDistance(distance);
		}

		float pitch = mInGameCameraController->GetPitch();
		if (ImGui::SliderFloat("Pitch (Radians)", &pitch, 0.0f, 1.2f))
		{
			mInGameCameraController->SetPitch(pitch);
		}

		float yaw = mInGameCameraController->GetYaw();
		ImGui::Text("Yaw: %.2f", yaw);
	}

	ImGui::End();
}
