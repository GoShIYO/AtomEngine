#include "Input.h"
#include "Runtime/Core/LogSystem/LogSystem.h"
#include "Runtime/Core/Utility/Utility.h"
#include "Runtime/Function/Render/WindowManager.h"
#include "imgui.h"

#include <Xinput.h>
#include <algorithm>
#include <cmath>
#include <cstring>

#pragma comment(lib, "gameinput.lib")
#pragma comment(lib, "Xinput.lib")

namespace
{
    GameInputGamepadButtons ConvertXInputButtons(WORD buttons)
    {
        GameInputGamepadButtons converted = static_cast<GameInputGamepadButtons>(0);

        if (buttons & XINPUT_GAMEPAD_A) converted |= GameInputGamepadA;
        if (buttons & XINPUT_GAMEPAD_B) converted |= GameInputGamepadB;
        if (buttons & XINPUT_GAMEPAD_X) converted |= GameInputGamepadX;
        if (buttons & XINPUT_GAMEPAD_Y) converted |= GameInputGamepadY;
        if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) converted |= GameInputGamepadLeftShoulder;
        if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) converted |= GameInputGamepadRightShoulder;
        if (buttons & XINPUT_GAMEPAD_LEFT_THUMB) converted |= GameInputGamepadLeftThumbstick;
        if (buttons & XINPUT_GAMEPAD_RIGHT_THUMB) converted |= GameInputGamepadRightThumbstick;
        if (buttons & XINPUT_GAMEPAD_DPAD_UP) converted |= GameInputGamepadDPadUp;
        if (buttons & XINPUT_GAMEPAD_DPAD_DOWN) converted |= GameInputGamepadDPadDown;
        if (buttons & XINPUT_GAMEPAD_DPAD_LEFT) converted |= GameInputGamepadDPadLeft;
        if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) converted |= GameInputGamepadDPadRight;
        if (buttons & XINPUT_GAMEPAD_START) converted |= GameInputGamepadMenu;
        if (buttons & XINPUT_GAMEPAD_BACK) converted |= GameInputGamepadView;

        return converted;
    }

    float NormalizeTrigger(BYTE value)
    {
        return static_cast<float>(value) / 255.0f;
    }

    float NormalizeThumb(SHORT value)
    {
        const float denominator = value < 0 ? 32768.0f : 32767.0f;
        return std::clamp(static_cast<float>(value) / denominator, -1.0f, 1.0f);
    }
}

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
        {
            deviceKindString = L"Gamepad";
        }
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
        const HRESULT hr = GameInputCreate(&mInput);
        if (SUCCEEDED(hr))
        {
            mGameInputSupported = true;

            const HRESULT callbackResult = mInput->RegisterDeviceCallback(
                nullptr,
                GameInputKindGamepad | GameInputKindKeyboard | GameInputKindMouse,
                GameInputDeviceAnyStatus,
                GameInputAsyncEnumeration,
                nullptr,
                &DeviceCallback,
                &mDeviceCallbackToken);

            if (FAILED(callbackResult))
            {
                mDeviceCallbackToken = 0;
                Log("[Input] Failed to register GameInput device callback. Continuing without notifications.\n");
            }
        }
        else
        {
            mGameInputSupported = false;
            mInput.Reset();
            Log("[Input] GameInput not available. Falling back to XInput.\n");
        }
    }

    void Input::Shutdown()
    {
        if (mGameInputSupported && mInput && mDeviceCallbackToken != 0)
        {
#if GAMEINPUT_API_VERSION >= 2
            mInput->UnregisterCallback(mDeviceCallbackToken);
#else
            mInput->UnregisterCallback(mDeviceCallbackToken, UINT64_MAX);
#endif
            mDeviceCallbackToken = 0;
        }

        mReading.Reset();
        mPrevReading.Reset();
        mInput.Reset();
        mGameInputSupported = false;
    }

    void Input::Update()
    {
        const bool hasGameInput = mGameInputSupported && mInput;

#pragma region keyboard入力
        std::memcpy(mPreKeys, mkeys, sizeof(mkeys));
        std::memset(mkeys, 0, sizeof(mkeys));
        mPrevMouseState = mMouseState;
        mPrevGamepadState = mGamepadState;
        mCursorInput = CursorInput{};

        bool keyboardUpdated = false;

        if (hasGameInput && SUCCEEDED(mInput->GetCurrentReading(GameInputKindKeyboard, nullptr, &mReading)))
        {
            const uint32_t count = mReading->GetKeyCount();
            if (count > 0)
            {
                GameInputKeyState keyboardState[16];

                if (mReading->GetKeyState(_countof(keyboardState), keyboardState))
                {
                    for (uint32_t i = 0; i < count; i++)
                    {
                        uint8_t virtualKey = keyboardState[i].virtualKey;
                        mkeys[virtualKey] = true;
                    }
                    keyboardUpdated = true;
                    mDeviceType = InputDeviceType::Keyboard;
                }
            }
        }

        if (!keyboardUpdated)
        {
            UpdateKeyboardStateWin32();
        }
#pragma endregion

#pragma region マウス入力
        bool mouseUpdated = false;

        if (hasGameInput && !ImGui::GetIO().WantCaptureMouse)
        {
            if (SUCCEEDED(mInput->GetCurrentReading(GameInputKindMouse, nullptr, &mReading)))
            {
                static int64_t lastDeltaX = 0;
                static int64_t lastDeltaY = 0;
                static int64_t lastDeltaWheel = 0;
                static GameInputMouseState lastState{};

                if (mReading->GetMouseState(&mMouseState))
                {
                    int64_t deltaX = mMouseState.positionX - lastState.positionX;
                    int64_t deltaY = mMouseState.positionY - lastState.positionY;
                    int64_t deltaWheel = mMouseState.wheelY - lastState.wheelY;

                    lastDeltaX = deltaX ? deltaX : lastDeltaX;
                    lastDeltaY = deltaY ? deltaY : lastDeltaY;
                    lastDeltaWheel = deltaWheel ? deltaWheel : lastDeltaWheel;

                    lastState = mMouseState;
                    mouseUpdated = true;

                    if (!mCursorInput.CursorInputSwitch)
                        mCursorInput.CursorInputSwitch = mMouseState.buttons & GameInputMouseRightButton;

                    if (mCursorInput.CursorInputRotation == 0)
                        mCursorInput.CursorInputRotation = static_cast<int>(deltaX);
                }
            }
        }

        if (!mouseUpdated)
        {
            UpdateMouseStateWin32();
        }
#pragma endregion

#pragma region GamePad
        bool gamepadUpdated = false;

        if (hasGameInput)
        {
            if (SUCCEEDED(mInput->GetCurrentReading(GameInputKindGamepad, nullptr, &mReading)))
            {
                if (mReading->GetGamepadState(&mGamepadState))
                {
                    gamepadUpdated = true;
                }
                else
                {
                    mGamepadState = {};
                }
            }
            else
            {
                mGamepadState = {};
            }
        }

        if (!gamepadUpdated)
        {
            gamepadUpdated = UpdateGamepadStateFromXInput();
        }

        if (gamepadUpdated)
        {
            mIsPadContact = true;

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

            mSticks.leftTrigger = mGamepadState.leftTrigger;
            mSticks.rightTrigger = mGamepadState.rightTrigger;
            mSticks.leftStickX = mGamepadState.leftThumbstickX;
            mSticks.leftStickY = mGamepadState.leftThumbstickY;
            mSticks.rightStickX = mGamepadState.rightThumbstickX;
            mSticks.rightStickY = mGamepadState.rightThumbstickY;
        }
        else
        {
            mIsPadContact = false;
            mSticks = {};
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
        case GamePadButton::LB:             flag = GameInputGamepadLeftShoulder; break;
        case GamePadButton::RB:             flag = GameInputGamepadRightShoulder; break;
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
        case GamePadButton::LB:             flag = GameInputGamepadLeftShoulder; break;
        case GamePadButton::RB:             flag = GameInputGamepadRightShoulder; break;
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
        return ((mGamepadState.buttons & flag) != 0) && ((mPrevGamepadState.buttons & flag) == 0);
    }

    bool Input::IsReleaseKey(uint8_t key) const
    {
        return  mPreKeys[key] && !mkeys[key];
    }

    bool Input::IsReleaseGamePad(GamePadButton button) const
    {
        GameInputGamepadButtons flag;
        switch (button)
        {
        case GamePadButton::A:              flag = GameInputGamepadA; break;
        case GamePadButton::B:              flag = GameInputGamepadB; break;
        case GamePadButton::X:              flag = GameInputGamepadX; break;
        case GamePadButton::Y:              flag = GameInputGamepadY; break;
        case GamePadButton::LB:             flag = GameInputGamepadLeftShoulder; break;
        case GamePadButton::RB:             flag = GameInputGamepadRightShoulder; break;
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
        return ((mPrevGamepadState.buttons & flag) != 0 && (mGamepadState.buttons & flag) == 0);
    }

    bool Input::GetMousePosition(int* mouseX, int* mouseY)
    {
        if (mouseX && mouseY)
        {
            POINT cursorPos;
            GetCursorPos(&cursorPos);

            auto window = WindowManager::GetInstance();
            ScreenToClient(window->GetWindowHandle(), &cursorPos);

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

    void Input::UpdateKeyboardStateWin32()
    {
        bool anyPressed = false;

        for (int key = 0; key < 256; ++key)
        {
            if (GetAsyncKeyState(key) & 0x8000)
            {
                mkeys[key] = true;
                anyPressed = true;
            }
        }

        if (anyPressed)
        {
            mDeviceType = InputDeviceType::Keyboard;
        }
    }

    void Input::UpdateMouseStateWin32()
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse)
        {
            return;
        }

        POINT cursorPos{};
        if (!GetCursorPos(&cursorPos))
        {
            return;
        }

        if (auto window = WindowManager::GetInstance())
        {
            ScreenToClient(window->GetWindowHandle(), &cursorPos);
        }

        mMouseState.positionX = cursorPos.x;
        mMouseState.positionY = cursorPos.y;

        uint32_t buttons = 0;
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) buttons |= GameInputMouseLeftButton;
        if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) buttons |= GameInputMouseRightButton;
        if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) buttons |= GameInputMouseMiddleButton;
        if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) buttons |= GameInputMouseButton4;
        if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) buttons |= GameInputMouseButton5;

        mMouseState.buttons = (GameInputMouseButtons)buttons;

        const int64_t deltaX = mMouseState.positionX - mPrevMouseState.positionX;
        const int64_t deltaY = mMouseState.positionY - mPrevMouseState.positionY;

        const float wheelDelta = io.MouseWheel;
        if (wheelDelta != 0.0f)
        {
            mMouseState.wheelY += static_cast<int64_t>(wheelDelta * WHEEL_DELTA);
        }

        if (!mCursorInput.CursorInputSwitch)
        {
            mCursorInput.CursorInputSwitch = (buttons & GameInputMouseRightButton) != 0;
        }

        if (mCursorInput.CursorInputRotation == 0)
        {
            mCursorInput.CursorInputRotation = static_cast<int>(deltaX);
        }

        mCursorInput.CursorInputMouseX = static_cast<int>(cursorPos.x);
        mCursorInput.CursorInputMouseY = static_cast<int>(cursorPos.y);

        if (buttons != 0 || deltaX != 0 || deltaY != 0)
        {
            mDeviceType = InputDeviceType::Mouse;
        }
    }

    bool Input::UpdateGamepadStateFromXInput()
    {
        XINPUT_STATE state{};
        for (DWORD userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex)
        {
            if (XInputGetState(userIndex, &state) == ERROR_SUCCESS)
            {
                mGamepadState.buttons = ConvertXInputButtons(state.Gamepad.wButtons);
                mGamepadState.leftTrigger = NormalizeTrigger(state.Gamepad.bLeftTrigger);
                mGamepadState.rightTrigger = NormalizeTrigger(state.Gamepad.bRightTrigger);
                mGamepadState.leftThumbstickX = NormalizeThumb(state.Gamepad.sThumbLX);
                mGamepadState.leftThumbstickY = NormalizeThumb(state.Gamepad.sThumbLY);
                mGamepadState.rightThumbstickX = NormalizeThumb(state.Gamepad.sThumbRX);
                mGamepadState.rightThumbstickY = NormalizeThumb(state.Gamepad.sThumbRY);
                return true;
            }
        }

        mGamepadState = {};
        return false;
    }
}