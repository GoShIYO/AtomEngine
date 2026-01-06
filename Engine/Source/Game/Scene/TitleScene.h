#pragma once
#include "Runtime/Function/Scene/Scene.h"
#include "Runtime/Function/Input/Input.h"
#include "Runtime/Function/Camera/CameraBase.h"

class TitleScene : public AtomEngine::Scene
{
public:
	TitleScene(std::string_view name, AtomEngine::SceneManager& manager);

	bool Initialize() override;
	void Update(float deltaTime) override;
	void Render() override;
	void Shutdown() override;
	bool Exit() override;

private:
	AtomEngine::Camera mGameCamera;
	AtomEngine::Input* mInput{ nullptr };
	
	float mTitleAnimTime{ 0.0f };
	bool mStartPressed{ false };
	float mFadeOutTimer{ 0.0f };
};

