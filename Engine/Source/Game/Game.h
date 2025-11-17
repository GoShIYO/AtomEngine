#pragma once
#include "Runtime/Application/GameApp.h"

class Game :
	public GameApp
{
public:
	void Initialize() override;
	void Update(float deltaTime) override;
	void Render()override;
	void Shutdown() override;
	bool Exit() override;
};
