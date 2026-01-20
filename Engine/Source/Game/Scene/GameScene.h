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
	Camera mGameCamera;
	std::unique_ptr<DebugCamera> mDebugCamera;
	bool mUseDebugCamera = true;
private:
	void DestroyGameObject();
	void ImGuiHandleObjects();

};

