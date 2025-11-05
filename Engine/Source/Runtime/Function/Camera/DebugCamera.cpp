#include "DebugCamera.h"
#include "Runtime/Function/Input/Input.h"

namespace AtomEngine
{
    DebugCamera::DebugCamera(Camera& camera)
        : CameraController(camera)
        , mMoveSpeed(5.0f)
        , mRotateSpeed(0.25f)
        , mWheelSpeed(0.5f)
        , mPreWheelY(0)
    {
        mCurrMousePos = { 0,0 };
        mPrevMousePos = { 0,0 };
        memset(mIsShifting, 0, sizeof(mIsShifting));
        memset(mShiftTimer, 0, sizeof(mShiftTimer));

        mCurrentPos = mTargetPos = mTargetCamera.GetPosition();
        mCurrentRot = mTargetRot = Quaternion::LookRotation(
            (Vector3::ZERO - mTargetCamera.GetPosition()).NormalizedCopy(), Vector3::UP);
    }

    void DebugCamera::Update(float deltaTime)
    {
        mInput->GetMousePosition(&mCurrMousePos.x, &mCurrMousePos.y);

        MoveView(deltaTime);
        RotateView(deltaTime);
        TranslateView(deltaTime);

        mCurrentPos = VInterpTo(mCurrentPos, mTargetPos, deltaTime, mInterpSpeed);
        mCurrentRot = Quaternion::Slerp(mCurrentRot, mTargetRot, deltaTime * mInterpSpeed);

        mTargetCamera.SetPosition(mCurrentPos);
        mTargetCamera.SetRotation(mCurrentRot);
        mTargetCamera.Update();

        mPrevMousePos = mCurrMousePos;
    }


    void DebugCamera::MoveView(float dt)
    {
        Vector3 forward = mTargetCamera.GetForwardVec();
        Vector3 right = mTargetCamera.GetRightVec();

        if (mInput->IsPressKey('W')) mTargetPos += forward * (mMoveSpeed * dt);
        if (mInput->IsPressKey('S')) mTargetPos -= forward * (mMoveSpeed * dt);
        if (mInput->IsPressKey('A')) mTargetPos -= right * (mMoveSpeed * dt);
        if (mInput->IsPressKey('D')) mTargetPos += right * (mMoveSpeed * dt);

        int64_t curWheelY = mInput->GetMouseState().wheelY;
        mTargetPos += forward * ((curWheelY - mPreWheelY) / 120.0f * mWheelSpeed);
        mPreWheelY = curWheelY;
    }

    void DebugCamera::RotateView(float dt)
    {
        if (mInput->IsTriggerMouse(1))
            mPrevMousePos = mCurrMousePos;
        if (mInput->IsPressMouse(1))
        {
            float dx = DirectX::XMConvertToRadians((mCurrMousePos.x - mPrevMousePos.x) * mRotateSpeed);
            float dy = DirectX::XMConvertToRadians((mCurrMousePos.y - mPrevMousePos.y) * mRotateSpeed);

            if (mInput->IsPressKey(VK_LMENU))
            {
                Vector3 lookAt = Vector3::ZERO;
                Vector3 toCamera = mTargetPos - lookAt;
                float radius = toCamera.Length();

                float yaw = Math::atan2(toCamera.x, toCamera.z);
                float pitch = Math::asin(toCamera.y / radius);

                yaw -= dx;
                pitch -= dy;

                constexpr float limit = DirectX::XMConvertToRadians(89.9f);
                pitch = std::clamp(pitch, -limit, limit);

                Vector3 newPos;
                newPos.x = lookAt.x + radius * sinf(yaw) * cosf(pitch);
                newPos.y = lookAt.y + radius * sinf(pitch);
                newPos.z = lookAt.z + radius * cosf(yaw) * cosf(pitch);

                Vector3 forward = (lookAt - newPos).NormalizedCopy();
                Vector3 right = Vector3::UP.Cross(forward).NormalizedCopy();
                Vector3 up = forward.Cross(right).NormalizedCopy();

                mTargetPos = newPos;
                mTargetRot.FromAxes(right, up, forward);
            }
            else
            {
                Quaternion q = mTargetRot;
                Quaternion rotY = Quaternion(Vector3::UP, dx);
                Quaternion rotX = Quaternion(mTargetCamera.GetRightVec(), dy);

                mTargetRot = q * rotY * rotX;
            }
        }
    }

    void DebugCamera::TranslateView(float dt)
    {
        if (mInput->IsTriggerMouse(2))
            mPrevMousePos = mCurrMousePos;

        if (!mInput->IsPressMouse(2))
            return;

        float dx = (mCurrMousePos.x - mPrevMousePos.x) * 0.5f;
        float dy = (mCurrMousePos.y - mPrevMousePos.y) * 0.5f;

        Vector3 right = mTargetCamera.GetRightVec();
        Vector3 up = mTargetCamera.GetUpVec();

        mTargetPos += (-dx * right + dy * up) * dt;
    }

    Vector3 DebugCamera::VInterpTo(const Vector3& current, const Vector3& target, float dt, float speed)
    {
        if (speed <= 0.0f) return target;

        Vector3 delta = target - current;
        float dist = delta.Length();

        if (dist < 0.0001f) return target;

        float move = dist * std::clamp(dt * speed, 0.0f, 1.0f);
        return current + delta.NormalizedCopy() * move;
    }
}