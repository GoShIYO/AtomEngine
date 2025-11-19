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

        Vector3 VInterpTo(const Vector3& current, const Vector3& target, float dt, float speed);
        Quaternion QInterpTo(const Quaternion& current, const Quaternion& target, float dt, float speed);

    protected:
        Camera& mTargetCamera;
        Input* mInput;
    private:
        CameraController& operator=(const CameraController&) { return *this; }
    };
}

