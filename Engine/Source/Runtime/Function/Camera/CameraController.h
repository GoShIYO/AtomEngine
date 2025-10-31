#pragma once
#include "CameraBase.h"

namespace AtomEngine
{
    class Input;
    class CameraController
    {
    public:
        CameraController(Camera& camera);
        virtual ~CameraController() {}
        virtual void Update(float dt) = 0;

        static void ApplyMomentum(float& oldValue, float& newValue, float deltaTime);

    protected:
        Camera& mTargetCamera;
        Input* mInput;
    private:
        CameraController& operator=(const CameraController&) { return *this; }
    };

    class SphericalCamera : public CameraController
    {
    public:
        SphericalCamera(Camera& camera)
            : CameraController(camera)
            , mTarget(Vector3::ZERO)
            , mRadius(5.0f)
            , mXRotate(0.0f)
            , mYRotate(0.0f)
            , mRotateSpeed(0.005f)
            , mZoomSpeed(1.0f)
        {
        }

        void SetTarget(const Vector3& target) { mTarget = target; }
        void SetRadius(float radius) { mRadius = std::max(radius, 0.1f); }
        void SetRotateSpeed(float speed) { mRotateSpeed = speed; }
        void SetZoomSpeed(float speed) { mZoomSpeed = speed; }

        void Update(float deltaTime) override;

    private:

        void HandleMouseInput(float deltaTime);

        void UpdateCamera();

    private:

        Vector3 mTarget;
        float mRadius;
        float mXRotate;
        float mYRotate;
        float mRotateSpeed;
        float mZoomSpeed;
        int64_t mPreWheelY = 0;
    };
}

