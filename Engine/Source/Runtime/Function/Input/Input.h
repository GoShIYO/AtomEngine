#pragma once
#include<GameInput.h>
#include<array>
#include <wrl.h>

#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#elif GAMEINPUT_API_VERSION == 2
using namespace GameInput::v2;
#endif

namespace AtomEngine
{
	enum GamePadButton
	{
		A = 0,
		B,
		X,
		Y,
		LB,
		RB,
		LeftThumbstick,
		RightThumbstick,
		DPadUp,
		DPadDown,
		DPadLeft,
		DPadRight,
		Menu,
		View,
		COUNT
	};

	struct PadStick
	{
		float leftTrigger;
		float rightTrigger;
		float leftStickX;
		float leftStickY;
		float rightStickX;
		float rightStickY;
	};

	enum class InputDeviceType
	{
		None,
		Keyboard,
		Mouse,
		Gamepad
	};

	class Input
	{
	public:
		static Input* GetInstance();

		void Initialize();
		void Shutdown();
		void Update();
		/// <summary>
		/// キーの押下をチェック
		/// </summary>
		/// <param name="key">キー番号( VK_W 等)</param>
		/// <returns>押されているか</returns>
		bool IsPressKey(uint8_t key)const;
		/// <summary>
		/// キーのトリガーをチェック
		/// </summary>
		/// <param name="key">キー番号( VK_W 等)</param>
		/// <returns>トリガーか</returns>
		bool IsTriggerKey(uint8_t key)const;
		/// <summary>
		/// マウスの押下をチェック
		/// </summary>
		/// <param name="button">マウスボタン番号(0:左,1:右,2:中,3~4:拡張マウスボタン)</param>
		/// <returns>押されているか</returns>
		bool IsPressMouse(int button)const;
		/// <summary>
		/// マウスのトリガーをチェック。押した瞬間だけtrueになる
		/// </summary>
		/// <param name="button">マウスボタン番号(0:左,1:右,2:中,3~4:拡張マウスボタン)</param>
		/// <returns>トリガーか</returns>
		bool IsTriggerMouse(int button)const;
		/// <summary>
		/// コントローラーのボタン押下をチェック
		/// </summary>
		/// <param name="button">ボタンの種類</param>
		/// <returns>押されているか</returns>
		bool IsPressGamePad(GamePadButton button)const;
		/// <summary>
		/// コントローラーのボタントリガーをチェック
		/// </summary>
		/// <param name="button">ボタンの種類</param>
		/// <returns>トリガーか</returns>
		bool IsTriggerGamePad(GamePadButton button)const;
		/// <summary>
		///	マウススクリーン上の位置を取得
		/// </summary>
		/// <param name="mouseX">X座標</param>
		/// <param name="mouseY">Y座標</param>
		/// <returns></returns>
		bool GetMousePosition(int* mouseX, int* mouseY);
		/// <summary>
		/// マウスの状態を取得
		/// </summary>
		/// <returns></returns>
		const GameInputMouseState& GetMouseState()const { return mMouseState; };
		/// <summary>
		/// ゲームパッドのスティック情報を取得します。
		/// </summary>
		/// <returns>現在のゲームパッドスティック（PadStick型）への参照。</returns>
		const PadStick& GetGamePadStick()const { return mSticks; };
		const GameInputGamepadButtons& GetGamePadButton()const { return mGamepadState.buttons; };
		bool IsUseGamePad()const;
		bool IsStickTriggerRepeat(float value, float threshold, float dt, float firstDelay, float repeatInterval) const;
	private:

		enum MouseButton
		{
			Left = 0,
			Right,
			Middle,
			Button4,
			Button5,
			NONE
		};

		struct CursorState
		{
			int xPos;
			int yPos;
			int degreeRotation;
			bool isRelativeMode;
			bool cursorVisible;

			CursorState() = default;
			CursorState(int x, int y)
			{
				xPos = x; yPos = y; degreeRotation = 0; isRelativeMode = true; cursorVisible = true;
			};
		};

		struct CursorInput
		{
			bool CursorInputSwitch = false;
			float CursorInputAxisX;
			float CursorInputAxisY;
			int CursorInputRotation;
			int CursorInputMouseX;
			int CursorInputMouseY;
		};

		struct StickRepeatState
		{
			bool pressed = false;
			float timer = 0.0f;
		};

		Microsoft::WRL::ComPtr<IGameInput> mInput;
		Microsoft::WRL::ComPtr<IGameInputReading>   mPrevReading;
		Microsoft::WRL::ComPtr<IGameInputReading>   mReading;

		GameInputCallbackToken mDeviceCallbackToken = 0;

		GameInputMouseState mMouseState;
		GameInputMouseState mPrevMouseState;

		GameInputGamepadState mGamepadState = {};
		GameInputGamepadState mPrevGamepadState = {};

		bool mkeys[256] = {};
		bool mPreKeys[256] = {};

		mutable StickRepeatState mLeftXState;
		PadStick mSticks{};

		CursorState mCursorState;
		CursorInput mCursorInput{};
		CursorInput mLastCursorInput{};

		GamePadButton mGamePadButton = GamePadButton::COUNT;
		InputDeviceType mDeviceType = InputDeviceType::None;
	private:
		Input() = default;
		~Input() = default;
		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;
	};
}
