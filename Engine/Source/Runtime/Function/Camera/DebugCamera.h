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
        
        Vector3 mCurrentPos;
        Vector3 mTargetPos;
        float mInterpSpeed = 8.0f;

        float mOrbitX = 0.0f;
        float mOrbitY = 0.0f;
        float mOrbitRadius = 10.0f;
        Vector3 mOrbitCenter = Vector3::ZERO;

        Vector3 mStartPos;
        Vector3 mStartLookAt;
        Quaternion mStartRot;
        Quaternion mCurrentRot;
        Quaternion mTargetRot;

    private:
        Vector3 VInterpTo(const Vector3& current, const Vector3& target, float dt, float speed);
    };
}
