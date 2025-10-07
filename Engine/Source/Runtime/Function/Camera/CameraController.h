#pragma once
#include "CameraBase.h"

namespace AtomEngine
{
    class CameraController
    {
    public:
        CameraController(Camera& camera) : m_TargetCamera(camera) {}
        virtual ~CameraController() {}
        virtual void Update(float dt) = 0;

        static void ApplyMomentum(float& oldValue, float& newValue, float deltaTime);

    protected:
        Camera& m_TargetCamera;

    private:
        CameraController& operator=(const CameraController&) { return *this; }
    };
}

