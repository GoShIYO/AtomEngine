#include "Input.h"
#include "Runtime/Core/LogSystem/LogSystem.h"
#include "Runtime/Core/Utility/Utility.h"
#include "Runtime/Function/Render/WindowManager.h"
#include "imgui.h"

#pragma comment(lib, "gameinput.lib")

namespace AtomEngine
{
	Input* Input::GetInstance()
	{
		static Input instance;
		return &instance;
	}

	void CALLBACK DeviceCallback(
		_In_ GameInputCallbackToken /*callbackToken*/,
		_In_ void* context,
		_In_ IGameInputDevice* device,
		_In_ uint64_t /*timestamp*/,
		_In_ GameInputDeviceStatus currentStatus,
		_In_ GameInputDeviceStatus previousStatus) noexcept
	{
		// [GameInput]
		// GameInputDeviceInfo contains general and type-specific information of a given device
		const GameInputDeviceInfo* info = nullptr;
#if GAMEINPUT_API_VERSION >= 2
		device->GetDeviceInfo(&info);
#else
		info = device->GetDeviceInfo();
#endif

		GameInputKind deviceKind = info->supportedInput;
		const wchar_t* deviceKindString;
		const wchar_t* deviceEventString = nullptr;

		if (deviceKind & GameInputKindGamepad)
			deviceKindString = L"Gamepad";
		else if (deviceKind & GameInputKindMouse)
			deviceKindString = L"Mouse";
		else
			deviceKindString = L"Keyboard";

		if ((previousStatus & GameInputDeviceConnected) == 0 &&
			(currentStatus & GameInputDeviceConnected) != 0)
		{
			deviceEventString = L"Connected";
		}
		else if ((previousStatus & GameInputDeviceConnected) != 0 &&
			(currentStatus & GameInputDeviceConnected) == 0)
		{
			deviceEventString = L"Disconnected";
		}
		Log("[DeviceCallback] %s device %s\n", deviceKindString, deviceEventString);
	}

	void Input::Initialize()
	{
		// GameInput初期化
		ThrowIfFailed(GameInputCreate(&mInput));
		// コールバック登録
		ThrowIfFailed(mInput->RegisterDeviceCallback(
			nullptr,
			GameInputKindGamepad | GameInputKindKeyboard | GameInputKindMouse,
			GameInputDeviceAnyStatus,
			GameInputAsyncEnumeration,
			nullptr,
			&DeviceCallback,
			&mDeviceCallbackToken)
		);
	}

	void Input::Shutdown()
	{
#if GAMEINPUT_API_VERSION >= 2
		mInput->UnregisterCallback(mDeviceCallbackToken);
#else
		mInput->UnregisterCallback(mDeviceCallbackToken, UINT64_MAX);
#endif
	}

	void Input::Update()
	{
		ImGuiIO& io = ImGui::GetIO();

#pragma region keyboard入力
		// キーの押す状態を更新
		std::memcpy(mPreKeys, mkeys, sizeof(mkeys));
		// 押す状態をリセット
		std::memset(mkeys, 0, sizeof(mkeys));
		mPrevMouseState = mMouseState;
		mPrevGamepadState = mGamepadState;
		mCursorInput = CursorInput{};

		if (SUCCEEDED(mInput->GetCurrentReading(GameInputKindKeyboard, nullptr, &mReading)))
		{
			// 押すキーの数
			const uint32_t count = mReading->GetKeyCount();
			if (count > 0)
			{
				GameInputKeyState keyboardState[16];

				// 押すキー状態
				if (mReading->GetKeyState(_countof(keyboardState), keyboardState))
				{
					for (uint32_t i = 0; i < count; i++)
					{
						uint8_t virtualKey = keyboardState[i].virtualKey;
						mkeys[virtualKey] = true;
					}
				}
				mDeviceType = InputDeviceType::Keyboard;
			}
		}
#pragma endregion
		if (!io.WantCaptureMouse)
		{
#pragma region マウス入力
			if (SUCCEEDED(mInput->GetCurrentReading(GameInputKindMouse, nullptr, &mReading)))
			{
				static int64_t lastDeltaX = 0;
				static int64_t lastDeltaY = 0;
				static int64_t lastDeltaWheel = 0;
				static GameInputMouseState lastState;

				if (mReading->GetMouseState(&mMouseState))
				{
					int64_t deltaX = mMouseState.positionX - lastState.positionX;
					int64_t deltaY = mMouseState.positionY - lastState.positionY;
					int64_t deltaWheel = mMouseState.wheelY - lastState.wheelY;

					lastDeltaX = deltaX ? deltaX : lastDeltaX;
					lastDeltaY = deltaY ? deltaY : lastDeltaY;
					lastDeltaWheel = deltaWheel ? deltaWheel : static_cast<int>(lastDeltaWheel);

					lastState = mMouseState;

					if (!mCursorInput.CursorInputSwitch)
						mCursorInput.CursorInputSwitch = mMouseState.buttons & GameInputMouseRightButton;

					if (mCursorInput.CursorInputRotation == 0)
						mCursorInput.CursorInputRotation = static_cast<int>(deltaX);
				}
			}
#pragma endregion
		}

#pragma region GamePad

		if (SUCCEEDED(mInput->GetCurrentReading(GameInputKindGamepad, nullptr, &mReading)))
		{
			if (mReading->GetGamepadState(&mGamepadState))
			{
				if (mGamepadState.buttons != 0 ||
					std::abs(mGamepadState.leftThumbstickX) > 0.1f ||
					std::abs(mGamepadState.leftThumbstickY) > 0.1f ||
					std::abs(mGamepadState.rightThumbstickX) > 0.1f ||
					std::abs(mGamepadState.rightThumbstickY) > 0.1f)
				{
					mDeviceType = InputDeviceType::Gamepad;
				}

				static int rotationMultiplier = 10;
				mCursorInput.CursorInputAxisX = mGamepadState.leftThumbstickX;
				mCursorInput.CursorInputAxisY = mGamepadState.leftThumbstickY;
				mCursorInput.CursorInputRotation = static_cast<int>(mGamepadState.rightThumbstickX * rotationMultiplier);

				// スティックとトリガーの値をメンバ変数に保存
				mSticks.leftTrigger = mGamepadState.leftTrigger;
				mSticks.rightTrigger = mGamepadState.rightTrigger;
				mSticks.leftStickX = mGamepadState.leftThumbstickX;
				mSticks.leftStickY = mGamepadState.leftThumbstickY;
				mSticks.rightStickX = mGamepadState.rightThumbstickX;
				mSticks.rightStickY = mGamepadState.rightThumbstickY;
			}
			else
			{
				// 読み取りに失敗した場合、現在の状態をクリア
				mGamepadState = {};
			}
		}
		else
		{
			// 読み取りに失敗した場合、現在の状態をクリア
			mGamepadState = {};
		}

#pragma endregion

		mLastCursorInput = mCursorInput;
	}

	bool Input::IsPressKey(uint8_t key)const
	{
		return mkeys[key];
	}

	bool Input::IsTriggerKey(uint8_t key)const
	{
		return mkeys[key] && !mPreKeys[key];
	}

	bool Input::IsPressMouse(int button)const
	{
		if (button < 0 || button >= NONE)return false;
		switch (button)
		{
		case MouseButton::Left:
			return mMouseState.buttons & GameInputMouseLeftButton;
		case MouseButton::Right:
			return mMouseState.buttons & GameInputMouseRightButton;
		case MouseButton::Middle:
			return mMouseState.buttons & GameInputMouseMiddleButton;
		case MouseButton::Button4:
			return mMouseState.buttons & GameInputMouseButton4;
		case MouseButton::Button5:
			return mMouseState.buttons & GameInputMouseButton5;
		}
		return false;
	}

	bool Input::IsTriggerMouse(int button)const
	{
		if (button < 0 || button >= NONE) return false;

		switch (button)
		{
		case MouseButton::Left:
			return (mMouseState.buttons & GameInputMouseLeftButton) &&
				!(mPrevMouseState.buttons & GameInputMouseLeftButton);
		case MouseButton::Right:
			return (mMouseState.buttons & GameInputMouseRightButton) &&
				!(mPrevMouseState.buttons & GameInputMouseRightButton);
		case MouseButton::Middle:
			return (mMouseState.buttons & GameInputMouseMiddleButton) &&
				!(mPrevMouseState.buttons & GameInputMouseMiddleButton);
		case MouseButton::Button4:
			return (mMouseState.buttons & GameInputMouseButton4) &&
				!(mPrevMouseState.buttons & GameInputMouseButton4);
		case MouseButton::Button5:
			return (mMouseState.buttons & GameInputMouseButton5) &&
				!(mPrevMouseState.buttons & GameInputMouseButton5);
		}
		return false;
	}

	bool Input::IsPressGamePad(GamePadButton button) const
	{
		GameInputGamepadButtons flag;
		switch (button)
		{
		case GamePadButton::A:              flag = GameInputGamepadA; break;
		case GamePadButton::B:              flag = GameInputGamepadB; break;
		case GamePadButton::X:              flag = GameInputGamepadX; break;
		case GamePadButton::Y:              flag = GameInputGamepadY; break;
		case GamePadButton::LB:				flag = GameInputGamepadLeftShoulder; break;
		case GamePadButton::RB:				flag = GameInputGamepadRightShoulder; break;
		case GamePadButton::LeftThumbstick: flag = GameInputGamepadLeftThumbstick; break;
		case GamePadButton::RightThumbstick:flag = GameInputGamepadRightThumbstick; break;
		case GamePadButton::DPadUp:         flag = GameInputGamepadDPadUp; break;
		case GamePadButton::DPadDown:       flag = GameInputGamepadDPadDown; break;
		case GamePadButton::DPadLeft:       flag = GameInputGamepadDPadLeft; break;
		case GamePadButton::DPadRight:      flag = GameInputGamepadDPadRight; break;
		case GamePadButton::Menu:           flag = GameInputGamepadMenu; break;
		case GamePadButton::View:           flag = GameInputGamepadView; break;
		default: return false;
		}

		return (mGamepadState.buttons & flag) != 0;
	}

	bool Input::IsTriggerGamePad(GamePadButton button) const
	{
		GameInputGamepadButtons flag;
		switch (button)
		{
		case GamePadButton::A:              flag = GameInputGamepadA; break;
		case GamePadButton::B:              flag = GameInputGamepadB; break;
		case GamePadButton::X:              flag = GameInputGamepadX; break;
		case GamePadButton::Y:              flag = GameInputGamepadY; break;
		case GamePadButton::LB:				flag = GameInputGamepadLeftShoulder; break;
		case GamePadButton::RB:				flag = GameInputGamepadRightShoulder; break;
		case GamePadButton::LeftThumbstick: flag = GameInputGamepadLeftThumbstick; break;
		case GamePadButton::RightThumbstick:flag = GameInputGamepadRightThumbstick; break;
		case GamePadButton::DPadUp:         flag = GameInputGamepadDPadUp; break;
		case GamePadButton::DPadDown:       flag = GameInputGamepadDPadDown; break;
		case GamePadButton::DPadLeft:       flag = GameInputGamepadDPadLeft; break;
		case GamePadButton::DPadRight:      flag = GameInputGamepadDPadRight; break;
		case GamePadButton::Menu:           flag = GameInputGamepadMenu; break;
		case GamePadButton::View:           flag = GameInputGamepadView; break;
		default: return false;
		}
		// 現在のフレームで押されていて、前のフレームで押されていないか
		return ((mGamepadState.buttons & flag) != 0) && ((mPrevGamepadState.buttons & flag) == 0);
	}

	bool Input::GetMousePosition(int* mouseX, int* mouseY)
	{
		if (mouseX && mouseY)
		{
			// スクリーンを座標系からウィンドウ座標に変換
			POINT cursorPos;
			GetCursorPos(&cursorPos); // スクリーンを得る

			auto window = WindowManager::GetInstance();
			ScreenToClient(window->GetWindowHandle(), &cursorPos); // ウィンドウ座標に変換

			*mouseX = static_cast<int>(cursorPos.x);
			*mouseY = static_cast<int>(cursorPos.y);
			return true;
		}

		return false;
	}

	bool Input::IsUseGamePad() const
	{
		return mDeviceType == InputDeviceType::Gamepad;
	}

	bool Input::IsStickTriggerRepeat(float value, float threshold, float dt, float firstDelay, float repeatInterval) const
	{
		StickRepeatState& state = mLeftXState;

		if (std::abs(value) > threshold)
		{
			if (!state.pressed)
			{
				state.pressed = true;
				state.timer = firstDelay;
				return true;
			}
			else
			{
				state.timer -= dt;
				if (state.timer <= 0.0f)
				{
					state.timer = repeatInterval;
					return true;
				}
			}
		}
		else
		{
			state.pressed = false;
			state.timer = 0.0f;
		}

		return false;
	}
}