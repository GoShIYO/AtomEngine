#pragma once
#include "CameraController.h"

namespace AtomEngine
{
    struct PadStick;
    class GamePadCamera : public CameraController
    {
    public:
        GamePadCamera(Camera& camera);

        void Update(float dt) override;

        void SetTarget(const Vector3& target) { mTarget = target; }
        void SetDistance(float dist) { mRadius = dist; }
        float GetDistance() const { return mRadius; }
        void SetLookSensitivity(float sens) { mLookSensitivity = sens; }
        void SetZoomSpeed(float speed) { mZoomSpeed = speed; }

    private:
        void UpdateOrbit(const PadStick& pad, float dt);
        void UpdateZoom(const PadStick& pad,float dt);

    private:
        Vector3     mTarget;
        float       mRadius = 8.0f;

        float       mYaw = 0.0f;
        float       mPitch = 0.0f;

        float       mLookSensitivity = 2.0f;
        float       mZoomSpeed = 5.0f;

        Vector3     mCurrentPos;
        Vector3     mTargetPos;
        Quaternion  mCurrentRot;
        Quaternion  mTargetRot;
    };
}
