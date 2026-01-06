#include "TitleScene.h"
#include "Runtime/Function/Scene/SceneManager.h"
#include "Runtime/Function/Global/GlobalContext.h"
#include "../System/SoundManaged.h"
#include <imgui.h>
#include <cmath>


using namespace AtomEngine;

TitleScene::TitleScene(std::string_view name, SceneManager& manager)
	: Scene(name, manager)
{
	mInput = Input::GetInstance();
}

bool TitleScene::Initialize()
{
	mCamera = &mGameCamera;
	mTitleAnimTime = 0.0f;
	mStartPressed = false;
	mFadeOutTimer = 0.0f;

	Audio::GetInstance()->Play(Sound::gSoundMap["bgm"], true);
	Audio::GetInstance()->SetVolume(Sound::gSoundMap["bgm"], 0.5f);

	return Scene::Initialize();
}

void TitleScene::Update(float deltaTime)
{
	mGameCamera.Update();
	mTitleAnimTime += deltaTime;

	if (mStartPressed)
	{
		mFadeOutTimer += deltaTime;

		// フェードアウト後にゲームシーンへ
		if (mFadeOutTimer > 0.5f)
		{
			extern std::unique_ptr<Scene> CreateGameScene(const std::string & name, SceneManager & manager);
			RequestReplaceScene(CreateGameScene("Game", mManager));
		}
		return;
	}

	// 入力チェック
	bool startGame = false;

	// キーボード入力
	if (mInput->IsTriggerKey(VK_RETURN) || mInput->IsTriggerKey(VK_SPACE))
	{
		startGame = true;
	}

	// ゲームパッド入力
	if (mInput->IsContactGamePad())
	{
		if (mInput->IsTriggerGamePad(GamePadButton::A) ||
			mInput->IsTriggerGamePad(GamePadButton::Menu))
		{
			startGame = true;
		}
	}

	if (startGame)
	{
		mStartPressed = true;
		Audio::GetInstance()->Play(Sound::gSoundMap["button"]);
	}
}

void TitleScene::Render()
{
	// タイトル画面UIの描画
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_AlwaysAutoResize;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 center = viewport->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowBgAlpha(0.0f);

	if (ImGui::Begin("Title", nullptr, windowFlags))
	{
		// タイトルロゴ（アニメーション付き）
		float bounce = std::sin(mTitleAnimTime * 2.0f) * 5.0f;
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + bounce);

		ImGui::SetWindowFontScale(4.0f);

		// タイトル色（虹色アニメーション）
		float hue = std::fmod(mTitleAnimTime * 0.2f, 1.0f);
		ImVec4 titleColor;
		ImGui::ColorConvertHSVtoRGB(hue, 0.8f, 1.0f, titleColor.x, titleColor.y, titleColor.z);
		titleColor.w = 1.0f;

		ImGui::TextColored(titleColor, "P AND ADVANCE");

		ImGui::SetWindowFontScale(1.0f);

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		// スタートプロンプト（点滅）
		if (!mStartPressed)
		{
			float alpha = (std::sin(mTitleAnimTime * 3.0f) + 1.0f) * 0.5f;
			ImVec4 promptColor = ImVec4(1.0f, 1.0f, 1.0f, alpha);

			ImGui::SetWindowFontScale(1.5f);
			ImGui::TextColored(promptColor, "Press ENTER or A Button to Start");
			ImGui::SetWindowFontScale(1.0f);
		}
		else
		{
			// スタート後のフェードアウト効果
			float fadeAlpha = 1.0f - (mFadeOutTimer / 0.5f);
			ImVec4 fadeColor = ImVec4(1.0f, 1.0f, 1.0f, fadeAlpha);

			ImGui::SetWindowFontScale(2.0f);
			ImGui::TextColored(fadeColor, "GAME START!");
			ImGui::SetWindowFontScale(1.0f);
		}

		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::SetWindowFontScale(0.8f);
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Controls:");
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  WASD / Left Stick - Move");
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  Arrow Keys / Right Stick - Camera");
		ImGui::SetWindowFontScale(1.0f);
	}
	ImGui::End();
}

void TitleScene::Shutdown()
{
	if (Audio::GetInstance()->IsPlaying(Sound::gSoundMap["bgm"]))
		Audio::GetInstance()->Stop(Sound::gSoundMap["bgm"]);
}

bool TitleScene::Exit()
{
	return false;
}