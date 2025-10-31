#pragma once
#include "CameraController.h"
#include "Runtime/Core/Math/MathInclude.h"

namespace AtomEngine
{
    class Input;
    class DebugCamera : public CameraController
    {
    public:
        DebugCamera(Camera& camera);

        void Update(float deltaTime) override;

    private:

        void MoveView(float dt);

        void RotateView(float dt);

        void TranslateView(float dt);

        void ShiftView(float dt);

        void BeginShift(int index);

    private:
        struct POINT
        {
            int x;
            int y;
        };
        POINT mCurrMousePos;
        POINT mPrevMousePos;

        float mMoveSpeed;
        float mRotateSpeed;
        float mWheelSpeed;
        int64_t mPreWheelY;

        bool mIsShifting[4];
        float mShiftTimer[4];
        Vector3 mCameraPos[4];
        Vector3 mCameraLookAt[4];
        Vector3 mCameraUp[4];
        Vector3 mStartPos;
        Vector3 mStartLookAt;
    };
}
