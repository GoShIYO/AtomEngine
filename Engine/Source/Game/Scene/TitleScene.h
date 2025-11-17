#pragma once
#include "Runtime/Function/Scene/Scene.h"

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

};

