#pragma once
#include "Runtime/Function/Scene/Scene.h"
#include "Runtime/Function/Input/Input.h"

class ClearScene : public AtomEngine::Scene
{
public:
	ClearScene(std::string_view name, AtomEngine::SceneManager& manager);

	bool Initialize() override;
	void Update(float deltaTime) override;
	void Render() override;
	void Shutdown() override;
	bool Exit() override;

	void SetGameResult(int collectedItems, int totalItems, float clearTime);

private:
	AtomEngine::Input* mInput{ nullptr };
	AtomEngine::Camera mGameCamera;
	int mCollectedItems{ 0 };
	int mTotalItems{ 0 };
	float mClearTime{ 0.0f };
	
	float mDisplayTimer{ 0.0f };
	bool mCanProceed{ false };
};
