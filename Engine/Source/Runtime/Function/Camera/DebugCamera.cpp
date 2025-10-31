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

        mCameraPos[0] = Vector3(0.0f, 0.0f, -10.0f); // 正面
        mCameraLookAt[0] = Vector3(0.0f, 0.0f, 0.0f);
        mCameraUp[0] = Vector3::UP;

        mCameraPos[1] = Vector3(10.0f, 0.0f, 0.0f); // 右
        mCameraLookAt[1] = Vector3(0.0f, 0.0f, 0.0f);
        mCameraUp[1] = Vector3::UP;

        mCameraPos[2] = Vector3(0.0f, 10.0f, 0.0f); // 上
        mCameraLookAt[2] = Vector3(0.0f, 0.0f, 0.0f);
        mCameraUp[2] = Vector3::FORWARD;

        mCameraPos[3] = Vector3(0.0f, -10.0f, 0.0f); // 下
        mCameraLookAt[3] = Vector3(0.0f, 0.0f, 0.0f);
        mCameraUp[3] = -Vector3::FORWARD;
    }

    void DebugCamera::Update(float deltaTime)
    {
        mInput->GetMousePosition(&mCurrMousePos.x, &mCurrMousePos.y);

        MoveView(deltaTime);
        RotateView(deltaTime);
        TranslateView(deltaTime);
        ShiftView(deltaTime);

        mPrevMousePos = mCurrMousePos;

        mTargetCamera.Update();
    }

    void DebugCamera::MoveView(float dt)
    {
        Vector3 pos = mTargetCamera.GetPosition();
        Vector3 forward = mTargetCamera.GetForwardVec();
        Vector3 right = mTargetCamera.GetRightVec();

        if (mInput->IsPressKey('W'))
            pos += forward * (mMoveSpeed * dt);
        if (mInput->IsPressKey('S'))
            pos -= forward * (mMoveSpeed * dt);
        if (mInput->IsPressKey('A'))
            pos -= right * (mMoveSpeed * dt);
        if (mInput->IsPressKey('D'))
            pos += right * (mMoveSpeed * dt);

        int64_t curWheelY = mInput->GetMouseState().wheelY;
        pos += forward * ((curWheelY - mPreWheelY) / 120.0f * mWheelSpeed);
        mPreWheelY = curWheelY;

        mTargetCamera.SetPosition(pos);
    }

    void DebugCamera::RotateView(float dt)
    {
        bool isPressAlt = mInput->IsPressKey(VK_LMENU);

        if (mInput->IsTriggerMouse(1))
            mPrevMousePos = mCurrMousePos;

        if (!mInput->IsPressMouse(1))
            return;

        float dx = DirectX::XMConvertToRadians((mCurrMousePos.x - mPrevMousePos.x) * mRotateSpeed);
        float dy = DirectX::XMConvertToRadians((mCurrMousePos.y - mPrevMousePos.y) * mRotateSpeed);

        Quaternion q = mTargetCamera.GetRotation();

        if (isPressAlt)
        {
            Vector3 pos = mTargetCamera.GetPosition();
            Vector3 lookAt = Vector3::ZERO;
            Vector3 toCamera = pos - lookAt;
            float radius = toCamera.Length();

            float yaw = atan2f(toCamera.x, toCamera.z);
            float pitch = asinf(toCamera.y / radius);

            yaw -= dx;
            pitch -= dy;

            constexpr float limit = DirectX::XMConvertToRadians(89.9f);
            pitch = std::clamp(pitch, -limit, limit);

            Vector3 newPos;
            newPos.x = lookAt.x + radius * sinf(yaw) * cosf(pitch);
            newPos.y = lookAt.y + radius * sinf(pitch);
            newPos.z = lookAt.z + radius * cosf(yaw) * cosf(pitch);

            mTargetCamera.SetLookAt(newPos, lookAt, Vector3::UP);
        }
        else
        {
            Quaternion rotY = Quaternion(Vector3::UP, dx);
            Quaternion rotX = Quaternion(Vector3::RIGHT, dy);
            q = q * rotY * rotX;

            mTargetCamera.SetRotation(q);
        }
    }


    void DebugCamera::TranslateView(float dt)
    {
        if (mInput->IsTriggerMouse(2))
            mPrevMousePos = mCurrMousePos;

        if (mInput->IsPressMouse(2))
        {
            float dx = 0.5f * (mCurrMousePos.x - mPrevMousePos.x);
            float dy = 0.5f * (mCurrMousePos.y - mPrevMousePos.y);

            Vector3 pos = mTargetCamera.GetPosition();
            Vector3 right = mTargetCamera.GetRightVec();
            Vector3 up = mTargetCamera.GetUpVec();

            pos += (-dx * right + dy * up) * dt;
            mTargetCamera.SetPosition(pos);
        }
    }

    void DebugCamera::ShiftView(float dt)
    {
        if (mInput->IsTriggerKey(VK_NUMPAD1)) BeginShift(0);
        if (mInput->IsTriggerKey(VK_NUMPAD3)) BeginShift(1);
        if (mInput->IsTriggerKey(VK_NUMPAD7)) BeginShift(2);
        if (mInput->IsTriggerKey(VK_NUMPAD9)) BeginShift(3);

        for (int i = 0; i < 4; ++i)
        {
            if (mIsShifting[i])
            {
                mShiftTimer[i] += dt * 3.0f;
                if (mShiftTimer[i] >= 1.0f)
                {
                    mShiftTimer[i] = 1.0f;
                    mIsShifting[i] = false;
                }

                Vector3 pos = Math::Lerp(mStartPos, mCameraPos[i], mShiftTimer[i]);
                Vector3 lookAt = Math::Lerp(mStartLookAt, mCameraLookAt[i], mShiftTimer[i]);
                Vector3 up = mCameraUp[i];

                mTargetCamera.SetLookAt(pos, lookAt, up);
            }
        }
    }

    void DebugCamera::BeginShift(int index)
    {
        mIsShifting[index] = true;
        mShiftTimer[index] = 0.0f;
        mStartPos = mTargetCamera.GetPosition();
        mStartLookAt = mStartPos + mTargetCamera.GetForwardVec();
    }
}