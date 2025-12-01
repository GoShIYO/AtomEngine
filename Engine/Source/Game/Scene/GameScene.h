#pragma once
#include "Runtime/Function/Scene/Scene.h"
#include "Runtime/Resource/AssetManager.h"
#include "Runtime/Function/Camera/GamePadCamera.h"
#include "Runtime/Function/Camera/DebugCamera.h"
#include "Runtime/Function/Light/LightManager.h"
#include "Runtime/Function/Sprite/Sprite.h"
#include "../fwd.h"

using namespace AtomEngine;

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

	VoxelWorld mVoxelWorld;
	
	
	std::unique_ptr<DebugCamera> mDebugCamera;
	bool useDebug = false;
private:
	void AddGameObject(std::unique_ptr<GameObject>&& object);
	void DestroyGameObject();
	void ImGuiHandleObjects();

	void InitSystems();

};

