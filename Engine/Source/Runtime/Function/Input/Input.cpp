#include "Input.h"
#include "Runtime/Core/Log/LogSystem.h"
#include "Runtime/Core/Utility/Utility.h"

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
#if GAMEINPUT_API_VERSION >= 1
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
		ThrowIfFailed(GameInputCreate(&input_));
		// コールバック登録
		ThrowIfFailed(input_->RegisterDeviceCallback(
			nullptr,
			GameInputKindGamepad | GameInputKindKeyboard | GameInputKindMouse,
			GameInputDeviceAnyStatus,
			GameInputAsyncEnumeration,
			nullptr,
			&DeviceCallback,
			&deviceCallbackToken_)
		);
	}

	void Input::Finalize()
	{
#if GAMEINPUT_API_VERSION >= 1
		input_->UnregisterCallback(deviceCallbackToken_);
#else
		input_->UnregisterCallback(deviceCallbackToken_, UINT64_MAX);
#endif
	}

	void Input::Update()
	{
#pragma region keyboard入力
		// キーの押す状態を更新
		std::memcpy(preKeys_, keys_, sizeof(keys_));
		// 押す状態をリセット
		std::memset(keys_, 0, sizeof(keys_));
		preMouseState_ = mouseState_;
		prevGamepadState_ = gamepadState_;
		cursorInput_ = CursorInput{};

		if (SUCCEEDED(input_->GetCurrentReading(GameInputKindKeyboard, nullptr, &reading_)))
		{
			// 押すキーの数
			const uint32_t count = reading_->GetKeyCount();
			if (count > 0)
			{
				GameInputKeyState keyboardState[16];

				// 押すキー状態
				if (reading_->GetKeyState(_countof(keyboardState), keyboardState))
				{
					for (uint32_t i = 0; i < count; i++)
					{
						uint8_t virtualKey = keyboardState[i].virtualKey;
						keys_[virtualKey] = true;
					}
				}
				deviceType_ = InputDeviceType::Keyboard;
			}
			//reading_->Release();
		}
#pragma endregion

#pragma region マウス入力
		if (SUCCEEDED(input_->GetCurrentReading(GameInputKindMouse, nullptr, &reading_)))
		{
			static int64_t lastDeltaX = 0;
			static int64_t lastDeltaY = 0;
			static int64_t lastDeltaWheel = 0;
			static GameInputMouseState lastState;

			if (reading_->GetMouseState(&mouseState_))
			{
				int64_t deltaX = mouseState_.positionX - lastState.positionX;
				int64_t deltaY = mouseState_.positionY - lastState.positionY;
				int64_t deltaWheel = mouseState_.wheelY - lastState.wheelY;

				lastDeltaX = deltaX ? deltaX : lastDeltaX;
				lastDeltaY = deltaY ? deltaY : lastDeltaY;
				lastDeltaWheel = deltaWheel ? deltaWheel : static_cast<int>(lastDeltaWheel);

				lastState = mouseState_;

				if (!cursorInput_.CursorInputSwitch)
					cursorInput_.CursorInputSwitch = mouseState_.buttons & GameInputMouseRightButton;

				if (cursorInput_.CursorInputRotation == 0)
					cursorInput_.CursorInputRotation = static_cast<int>(deltaX);
			}
		}
#pragma endregion

#pragma region GamePad

		if (SUCCEEDED(input_->GetCurrentReading(GameInputKindGamepad, nullptr, &reading_)))
		{
			if (reading_->GetGamepadState(&gamepadState_))
			{
				if (gamepadState_.buttons != 0 ||
					std::abs(gamepadState_.leftThumbstickX) > 0.1f ||
					std::abs(gamepadState_.leftThumbstickY) > 0.1f ||
					std::abs(gamepadState_.rightThumbstickX) > 0.1f ||
					std::abs(gamepadState_.rightThumbstickY) > 0.1f)
				{
					deviceType_ = InputDeviceType::Gamepad;
				}

				static int rotationMultiplier = 10;
				cursorInput_.CursorInputAxisX = gamepadState_.leftThumbstickX;
				cursorInput_.CursorInputAxisY = gamepadState_.leftThumbstickY;
				cursorInput_.CursorInputRotation = static_cast<int>(gamepadState_.rightThumbstickX * rotationMultiplier);

				// スティックとトリガーの値をメンバ変数に保存
				sticks_.leftTrigger = gamepadState_.leftTrigger;
				sticks_.rightTrigger = gamepadState_.rightTrigger;
				sticks_.leftStickX = gamepadState_.leftThumbstickX;
				sticks_.leftStickY = gamepadState_.leftThumbstickY;
				sticks_.rightStickX = gamepadState_.rightThumbstickX;
				sticks_.rightStickY = gamepadState_.rightThumbstickY;
			}
			else
			{
				// 読み取りに失敗した場合、現在の状態をクリア
				gamepadState_ = {};
			}
		}
		else
		{
			// 読み取りに失敗した場合、現在の状態をクリア
			gamepadState_ = {};
		}

#pragma endregion

		lastCursorInput_ = cursorInput_;
	}

	bool Input::IsPressKey(uint8_t key)const
	{
		return keys_[key];
	}

	bool Input::IsTriggerKey(uint8_t key)const
	{
		return keys_[key] && !preKeys_[key];
	}

	bool Input::IsPressMouse(int button)const
	{
		if (button < 0 || button >= NONE)return false;
		switch (button)
		{
		case MouseButton::Left:
			return mouseState_.buttons & GameInputMouseLeftButton;
		case MouseButton::Right:
			return mouseState_.buttons & GameInputMouseRightButton;
		case MouseButton::Middle:
			return mouseState_.buttons & GameInputMouseMiddleButton;
		case MouseButton::Button4:
			return mouseState_.buttons & GameInputMouseButton4;
		case MouseButton::Button5:
			return mouseState_.buttons & GameInputMouseButton5;
		}
		return false;
	}

	bool Input::IsTriggerMouse(int button)const
	{
		if (button < 0 || button >= NONE) return false;

		switch (button)
		{
		case MouseButton::Left:
			return (mouseState_.buttons & GameInputMouseLeftButton) &&
				!(preMouseState_.buttons & GameInputMouseLeftButton);
		case MouseButton::Right:
			return (mouseState_.buttons & GameInputMouseRightButton) &&
				!(preMouseState_.buttons & GameInputMouseRightButton);
		case MouseButton::Middle:
			return (mouseState_.buttons & GameInputMouseMiddleButton) &&
				!(preMouseState_.buttons & GameInputMouseMiddleButton);
		case MouseButton::Button4:
			return (mouseState_.buttons & GameInputMouseButton4) &&
				!(preMouseState_.buttons & GameInputMouseButton4);
		case MouseButton::Button5:
			return (mouseState_.buttons & GameInputMouseButton5) &&
				!(preMouseState_.buttons & GameInputMouseButton5);
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

		return (gamepadState_.buttons & flag) != 0;
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
		return ((gamepadState_.buttons & flag) != 0) && ((prevGamepadState_.buttons & flag) == 0);
	}

	bool Input::IsUseGamePad() const
	{
		return deviceType_ == InputDeviceType::Gamepad;
	}

	bool Input::IsStickTriggerRepeat(float value, float threshold, float dt, float firstDelay, float repeatInterval) const
	{
		StickRepeatState& state = leftXState_;

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