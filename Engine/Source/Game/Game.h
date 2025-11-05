#pragma once
#include "Runtime/Application/GameApp.h"
#include "Runtime/Resource/AssetManager.h"
#include "Runtime/Function/Camera/DebugCamera.h"
#include "Runtime/Function/Render/SkyboxRenderer.h"
#include "Runtime/Function/Camera/ShadowCamera.h"

class Game :
	public GameApp
{
public:
	void Initialize() override;
	void Update(float deltaTime) override;
	void Render()override;
	void Shutdown() override;
	bool Exit() override;
private:
	std::unique_ptr<GameObject> m_GameObject;
	TextureRef test;
	DescriptorHandle gpuHandle;

	std::unique_ptr<DebugCamera> m_DebugCamera;
};
