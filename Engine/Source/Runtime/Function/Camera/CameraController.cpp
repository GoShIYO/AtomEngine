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

    void SphericalCamera::Update(float deltaTime)
    {
        HandleMouseInput(deltaTime);
        UpdateCamera();
    }

    void SphericalCamera::HandleMouseInput(float deltaTime)
    {
        const GameInputMouseState& mouseState = mInput->GetMouseState();

        float dx = static_cast<float>(mouseState.positionX);
        float dy = static_cast<float>(mouseState.positionY);

        if (mInput->IsPressMouse(0) || mInput->IsPressMouse(1))
        {
            if (mInput->IsPressMouse(0))
            {
                mXRotate += dx * mRotateSpeed;
                mYRotate += dy * mRotateSpeed;

                mYRotate = std::clamp(mYRotate, -Math::HalfPI + 0.1f, Math::HalfPI - 0.1f);
            }
        }

        int64_t curWheelY = mouseState.wheelY;
        float wheelDelta = static_cast<float>(curWheelY - mPreWheelY);
        mPreWheelY = curWheelY;

        if (wheelDelta != 0.0f)
        {
            mRadius -= wheelDelta / 120.0f * mZoomSpeed;
            mRadius = std::max(0.5f, mRadius);
        }
    }

    void SphericalCamera::UpdateCamera()
    {
        float x = mRadius * cosf(mYRotate) * sinf(mXRotate);
        float y = mRadius * sinf(mYRotate);
        float z = mRadius * cosf(mYRotate) * cosf(mXRotate);

        Vector3 eye = { x, y, z };
        mTargetCamera.SetLookAt(eye + mTarget, mTarget, Vector3::UP);
        mTargetCamera.Update();
    }
}

