#include "CameraController.h"

namespace AtomEngine
{
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

