#pragma once

class GameApp
{
public:
	virtual ~GameApp() = default;

	virtual void Initialize() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Render() = 0;
	virtual void Shutdown() = 0;
	virtual bool Exit();
};
