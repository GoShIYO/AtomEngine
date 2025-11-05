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
}

