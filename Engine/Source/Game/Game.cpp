#include "Game.h"
#include "Scene/TitleScene.h"
#include "Scene/GameScene.h"
#include "Scene/ClearScene.h"
#include "Game/System/SoundManaged.h"
std::unique_ptr<AtomEngine::Scene> CreateGameScene(const std::string& name, AtomEngine::SceneManager& manager)
{
	return std::make_unique<GameScene>(name, manager);
}

std::unique_ptr<AtomEngine::Scene> CreateTitleScene(const std::string& name, AtomEngine::SceneManager& manager)
{
	return std::make_unique<TitleScene>(name, manager);
}

std::unique_ptr<AtomEngine::Scene> CreateClearScene(const std::string& name, AtomEngine::SceneManager& manager)
{
	return std::make_unique<ClearScene>(name, manager);
}

void SetClearSceneGameResult(AtomEngine::Scene* scene, int collected, int total, float time)
{
	if (auto* clearScene = dynamic_cast<ClearScene*>(scene))
	{
		clearScene->SetGameResult(collected, total, time);
	}
}

void Game::Initialize()
{
	Sound::LoadSounds();

	auto titleScene = std::make_unique<TitleScene>("TitleScene", mSceneManager);
	mSceneManager.PushScene(std::move(titleScene));
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


