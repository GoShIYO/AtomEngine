#pragma once
#include <GameInput.h>
#ifndef GAMEINPUT_API_VERSION
#define GAMEINPUT_API_VERSION 0
#endif

#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#elif GAMEINPUT_API_VERSION == 2
using namespace GameInput::v2;
#elif GAMEINPUT_API_VERSION == 3
using namespace GameInput::v3;
#endif

#include <array>
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
        void Shutdown();
        void Update();
        bool IsPressKey(uint8_t key)const;
        bool IsTriggerKey(uint8_t key)const;
        bool IsPressMouse(int button)const;
        bool IsTriggerMouse(int button)const;
        bool IsPressGamePad(GamePadButton button)const;
        bool IsTriggerGamePad(GamePadButton button)const;
        bool IsReleaseKey(uint8_t key)const;
        bool IsReleaseGamePad(GamePadButton button)const;
        bool GetMousePosition(int* mouseX, int* mouseY);
        const GameInputMouseState& GetMouseState()const { return mMouseState; };
        const PadStick& GetGamePadStick()const { return mSticks; };
        const GameInputGamepadButtons& GetGamePadButton()const { return mGamepadState.buttons; };
        bool IsUseGamePad()const;
        bool IsStickTriggerRepeat(float value, float threshold, float dt, float firstDelay, float repeatInterval) const;
        bool IsContactGamePad() { return mIsPadContact; }
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

        void UpdateKeyboardStateWin32();
        void UpdateMouseStateWin32();
        bool UpdateGamepadStateFromXInput();

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

        bool mIsPadContact = false;
        bool mGameInputSupported = false;

        GamePadButton mGamePadButton = GamePadButton::COUNT;
        InputDeviceType mDeviceType = InputDeviceType::None;

    private:
        Input() = default;
        ~Input() = default;
        Input(const Input&) = delete;
        Input& operator=(const Input&) = delete;
    };
}