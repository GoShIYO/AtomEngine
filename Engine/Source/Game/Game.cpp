#include "Game.h"
#include "Scene/TitleScene.h"
#include "Scene/GameScene.h"

void Game::Initialize()
{
	auto gameScene = std::make_unique<GameScene>("GameScene", mSceneManager);
	mSceneManager.PushScene(std::move(gameScene));
}

void Game::Update(float deltaTime)
{
	mSceneManager.Update(deltaTime);
}

void Game::Render()
{
	mSceneManager.Render();
}

void Game::Shutdown()
{
	mSceneManager.Shutdown();
}

bool Game::Exit()
{
	return GameApp::Exit() || mSceneManager.Exit();
}


