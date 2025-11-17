#include "TitleScene.h"
#include "Runtime/Function/Scene/SceneManager.h"

using namespace AtomEngine;

TitleScene::TitleScene(std::string_view name, SceneManager& manager)
	: Scene(name, manager)
{
}

bool TitleScene::Initialize()
{



	return Scene::Initialize();
}

void TitleScene::Update(float deltaTime)
{
}

void TitleScene::Render()
{
}

void TitleScene::Shutdown()
{
}

bool TitleScene::Exit()
{
	return false;
}