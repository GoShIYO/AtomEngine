#include"SceneManager.h"

namespace AtomEngine
{
	void SceneManager::Update(float deltaTime)
	{
		Scene* current = GetCurrentScene();
		if (!current || !current->IsInitialized())
			return;

		current->Update(deltaTime);

		ProcessSceneChange();
	}

	void SceneManager::Render()
	{
		Scene* current = GetCurrentScene();
		if (current && current->IsInitialized())
			current->Render();
	}

	void SceneManager::Shutdown()
	{
		for (auto& scene : mScenes)
			if (scene)
				scene->Shutdown();

		mScenes.clear();
	}

	bool SceneManager::Exit()
	{
		Scene* current = GetCurrentScene();
		if (current && current->IsInitialized())
			return current->Exit();
		return false;
	}

	Scene* SceneManager::GetCurrentScene() const
	{
		if (mScenes.empty()) return nullptr;
		return mScenes.back().get();
	}

	void SceneManager::OnPopScene()
	{
		mSceneState = SceneState::In;
	}

	void SceneManager::OnPushScene(std::unique_ptr<Scene> scene)
	{
		mPendingScene = std::move(scene);
		mSceneState = SceneState::out;
	}

	void SceneManager::OnReplaceScene(std::unique_ptr<Scene> scene)
	{
		mPendingScene = std::move(scene);
		mSceneState = SceneState::Change;
	}

	void SceneManager::ProcessSceneChange()
	{
		if (mSceneState == SceneState::None)
			return;

		switch (mSceneState)
		{
		case SceneState::In:
			PushScene(std::move(mPendingScene));
			break;

		case SceneState::out:
			PopScene();
			break;

		case SceneState::Change:
			ReplaceScene(std::move(mPendingScene));
			break;
		}

		mPendingScene.reset();
		mSceneState = SceneState::None;
	}

	void SceneManager::PushScene(std::unique_ptr<Scene>&& scene)
	{
		if (!scene)
			return;

		if (scene->Initialize())
		{
			mScenes.emplace_back(std::move(scene));
		}
	}

	void SceneManager::PopScene()
	{
		if (mScenes.empty())
			return;

		auto& top = mScenes.back();
		top->Shutdown();
		mScenes.pop_back();

		if (mPendingScene)
		{
			PushScene(std::move(mPendingScene));
			mPendingScene.reset();
		}
	}

	void SceneManager::ReplaceScene(std::unique_ptr<Scene>&& scene)
	{
		if (!mScenes.empty())
		{
			mScenes.back()->Shutdown();
			mScenes.pop_back();
		}

		PushScene(std::move(scene));
	}
}