#pragma once
#include "Runtime/Function/Scene/Scene.h"
#include "Runtime/Resource/AssetManager.h"
#include "Runtime/Function/Camera/GamePadCamera.h"
#include "Runtime/Function/Camera/DebugCamera.h"
#include "Runtime/Function/Light/LightManager.h"
#include "Runtime/Function/Sprite/Sprite.h"
#include "Runtime/Function/Framework/ECS/ECSCommon.h"
#include "../Camera/InGameCameraController.h"

#include "../fwd.h"

using namespace AtomEngine;

class ItemSystem;
class GoalSystem;
class ItemGoalEditor;

class GameScene : public Scene
{
public:
	GameScene(std::string_view name, AtomEngine::SceneManager& manager);
	bool Initialize() override;
	void Update(float deltaTime) override;
	void Render() override;
	void Shutdown() override;
	bool Exit() override;

private:
	std::unordered_map<Entity, std::unique_ptr<GameObject>> mGameObjects;
	Camera mGameCamera;

	std::unique_ptr<PlayerSystem> mPlayerSystem;
	std::unique_ptr<MoveSystem> mMoveSystem;
	std::unique_ptr<VoxelCollisionSystem> mVoxelCollisionSystem;
	std::unique_ptr<PlatformSystem> mPlatformSystem;
	std::unique_ptr<PlatformEditor> mPlatformEditor;
	std::unique_ptr<LadderSystem> mLadderSystem;
	std::unique_ptr<LadderEditor> mLadderEditor;
	std::unique_ptr<ItemSystem> mItemSystem;
	std::unique_ptr<GoalSystem> mGoalSystem;
	std::unique_ptr<ItemGoalEditor> mItemGoalEditor;

	Entity mVoxelWorldEntity{ entt::null };
	Entity mPlayerEntity{ entt::null };

	std::unique_ptr<DebugCamera> mDebugCamera;
	std::unique_ptr<InGameCameraController> mInGameCameraController;
	bool mUseDebugCamera = false;
	bool mShowPlatformEditor = true;
	bool mShowItemGoalEditor = true;
	
	// ゲーム状態
	float mGameTime{ 0.0f };
	bool mIsGameClear{ false };
	float mClearDelayTimer{ 0.0f };
	
private:
	void AddGameObject(std::unique_ptr<GameObject>&& object);
	void DestroyGameObject();
	void ImGuiHandleObjects();

	void InitSystems();
	void CreateTestPlatforms();
	
	// ゲームUI描画
	void RenderGameUI();
	
	void OnGameClear();
};

