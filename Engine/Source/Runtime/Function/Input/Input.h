#pragma once
#include<GameInput.h>
#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#elif GAMEINPUT_API_VERSION == 2
using namespace GameInput::v2;
#endif

#include<array>
#include <wrl.h>

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
		void Finalize();
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
		/// マウスの状態を取得
		/// </summary>
		/// <returns></returns>
		const GameInputMouseState& GetMouseState()const { return mouseState_; };
		/// <summary>
		/// ゲームパッドのスティック情報を取得します。
		/// </summary>
		/// <returns>現在のゲームパッドスティック（PadStick型）への参照。</returns>
		const PadStick& GetGamePadStick()const { return sticks_; };
		const GameInputGamepadButtons& GetGamePadButton()const { return gamepadState_.buttons; };
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

		Microsoft::WRL::ComPtr<IGameInput> input_;
		Microsoft::WRL::ComPtr<IGameInputReading>   prevReading_;
		Microsoft::WRL::ComPtr<IGameInputReading>   reading_;

		GameInputCallbackToken deviceCallbackToken_ = 0;

		GameInputMouseState mouseState_;
		GameInputMouseState preMouseState_;

		GameInputGamepadState gamepadState_ = {};
		GameInputGamepadState prevGamepadState_ = {};

		bool keys_[256] = {};
		bool preKeys_[256] = {};

		mutable StickRepeatState leftXState_;
		PadStick sticks_{};

		CursorState cursorState_;
		CursorInput cursorInput_{};
		CursorInput lastCursorInput_{};

		GamePadButton gamePadButton_ = GamePadButton::COUNT;
		InputDeviceType deviceType_ = InputDeviceType::None;
	private:
		Input() = default;
		~Input() = default;
		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;
	};
}
