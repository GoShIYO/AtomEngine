#include "CameraController.h"
#include "Runtime/Function/Input/Input.h"
#undef min
#undef max

namespace AtomEngine
{
    CameraController::CameraController(Camera& camera) : mTargetCamera(camera)
    {
        mInput = Input::GetInstance();
    }
    void CameraController::ApplyMomentum(float& oldValue, float& newValue, float deltaTime)
    {
        float blendedValue;
        if (Math::Abs(newValue) > Math::Abs(oldValue))
            blendedValue = Math::Lerp(newValue, oldValue, Math::Pow(0.6f, deltaTime * 60.0f));
        else
            blendedValue = Math::Lerp(newValue, oldValue, Math::Pow(0.8f, deltaTime * 60.0f));
        oldValue = blendedValue;
        newValue = blendedValue;
    }
}

