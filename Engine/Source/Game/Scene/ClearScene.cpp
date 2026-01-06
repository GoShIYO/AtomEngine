#include "ClearScene.h"
#include "Runtime/Function/Scene/SceneManager.h"
#include "Runtime/Function/Global/GlobalContext.h"
#include "../System/SoundManaged.h"

#include <imgui.h>
#include <cmath>

using namespace AtomEngine;

ClearScene::ClearScene(std::string_view name, SceneManager& manager)
	: Scene(name, manager)
{
	mInput = Input::GetInstance();
}

bool ClearScene::Initialize()
{
	mCamera = &mGameCamera;
	mDisplayTimer = 0.0f;
	mCanProceed = false;

	return Scene::Initialize();
}

void ClearScene::Update(float deltaTime)
{
	mGameCamera.Update();

	mDisplayTimer += deltaTime;

	// 1秒後に入力受付開始
	if (mDisplayTimer > 1.0f)
	{
		mCanProceed = true;
	}

	// 入力でタイトルに戻る
	if (mCanProceed)
	{
		bool shouldReturn = false;

		// キーボード入力
		if (mInput->IsTriggerKey(VK_RETURN) || mInput->IsTriggerKey(VK_SPACE))
		{
			shouldReturn = true;
		}

		// ゲームパッド入力
		if (mInput->IsContactGamePad())
		{
			if (mInput->IsTriggerGamePad(GamePadButton::A) ||
				mInput->IsTriggerGamePad(GamePadButton::B))
			{
				shouldReturn = true;
			}
		}

		if (shouldReturn)
		{
			extern std::unique_ptr<Scene> CreateTitleScene(const std::string & name, SceneManager & manager);
			RequestReplaceScene(CreateTitleScene("Title", mManager));
		}
	}
}

void ClearScene::Render()
{
	// クリア画面UIの描画
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_AlwaysAutoResize;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 center = viewport->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowBgAlpha(0.8f);

	if (ImGui::Begin("Clear", nullptr, windowFlags))
	{
		// タイトル
		ImGui::PushFont(nullptr);  // 大きいフォントがあれば設定
		ImGui::SetWindowFontScale(3.0f);

		ImVec4 goldColor = ImVec4(1.0f, 0.84f, 0.0f, 1.0f);
		ImGui::TextColored(goldColor, "STAGE CLEAR!");

		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopFont();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// 結果表示
		ImGui::SetWindowFontScale(1.5f);

		ImGui::Text("Items Collected: %d / %d", mCollectedItems, mTotalItems);

		int minutes = static_cast<int>(mClearTime) / 60;
		int seconds = static_cast<int>(mClearTime) % 60;
		int milliseconds = static_cast<int>((mClearTime - static_cast<int>(mClearTime)) * 100);
		ImGui::Text("Clear Time: %02d:%02d.%02d", minutes, seconds, milliseconds);

		// 全アイテム収集ボーナス
		if (mCollectedItems == mTotalItems && mTotalItems > 0)
		{
			ImGui::Spacing();
			ImVec4 rainbowColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
			ImGui::TextColored(rainbowColor, "PERFECT!");
		}

		ImGui::SetWindowFontScale(1.0f);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// 続行プロンプト
		if (mCanProceed)
		{
			float alpha = (std::sin(mDisplayTimer * 3.0f) + 1.0f) * 0.5f;
			ImVec4 promptColor = ImVec4(1.0f, 1.0f, 1.0f, alpha);
			ImGui::TextColored(promptColor, "Press ENTER or A Button to return to Title");
		}
	}
	ImGui::End();
}

void ClearScene::Shutdown()
{
	if (Audio::GetInstance()->IsPlaying(Sound::gSoundMap["bgm"]))
		Audio::GetInstance()->Stop(Sound::gSoundMap["bgm"]);
}

bool ClearScene::Exit()
{
	return false;
}

void ClearScene::SetGameResult(int collectedItems, int totalItems, float clearTime)
{
	mCollectedItems = collectedItems;
	mTotalItems = totalItems;
	mClearTime = clearTime;
}
