#include "DebugCamera.h"
#include "Runtime/Function/Input/Input.h"

namespace AtomEngine
{
	DebugCamera::DebugCamera(Camera& camera)
		: CameraController(camera)
		, mMoveSpeed(5.0f)
		, mRotateSpeed(0.005f)
		, mPanSpeed(0.002f)
		, mZoomSpeed(1.0f)
	{
		mCurrentPos = camera.GetPosition();
		mTargetPos = mCurrentPos;
		mCurrentRot = camera.GetRotation();
		mTargetRot = mCurrentRot;
	}

	void DebugCamera::Update(float deltaTime)
	{
		if (mMode == CameraMode::Fly && mPrevMode == CameraMode::Orbit)
		{
			mRadius = (mTarget - mCurrentPos).Length();
		}
		else if (mMode == CameraMode::Orbit && mPrevMode == CameraMode::Fly)
		{
			Vector3 forward = mCurrentRot * Vector3::FORWARD;
			Vector3 newTarget = mCurrentPos + forward * mRadius;
			mTarget = VInterpTo(mTarget, newTarget, deltaTime, 10.0f);

			Vector3 offset = mCurrentPos - mTarget;
			mRadius = offset.Length();
			if (mRadius > 0.0001f)
			{
				mYaw = atan2f(offset.x, offset.z);
				mPitch = asinf(offset.y / mRadius);
			}
		}
		mPrevMode = mMode;

		int mouseX, mouseY;
		mInput->GetMousePosition(&mouseX, &mouseY);

		if (mFirstFrame)
		{
			mPrevMouseX = mouseX;
			mPrevMouseY = mouseY;
			mFirstFrame = false;
		}

		float dx = float(mouseX - mPrevMouseX);
		float dy = float(mouseY - mPrevMouseY);
		mPrevMouseX = mouseX;
		mPrevMouseY = mouseY;

		bool isLeftMouse = mInput->IsPressMouse(0);
		bool isMiddleMouse = mInput->IsPressMouse(2);
		bool isRightMouse = mInput->IsPressMouse(1);
		bool isShift = mInput->IsPressKey(VK_LSHIFT);
		if (isShift)
			mMode = CameraMode::Fly;
		else
			mMode = CameraMode::Orbit;

		if (mMode == CameraMode::Orbit)
		{
			//左クリック(回転)
			if (isLeftMouse)
				UpdateOrbit(dx, dy);
			//中クリック(移動)
			else if (isMiddleMouse)
				UpdatePan(dx, dy);
			//右クリック(ズーム)
			else if (isRightMouse)
				UpdateZoom(dy);

			float wheel = (float)mInput->GetMouseState().wheelY;
			if (wheel != 0.0f)
				UpdateZoom((wheel - mPrevMouseWheel) * 0.05f);
			mPrevMouseWheel = wheel;

			Vector3 eye;
			eye.x = mTarget.x + mRadius * cos(mPitch) * sin(mYaw);
			eye.y = mTarget.y + mRadius * sin(mPitch);
			eye.z = mTarget.z - mRadius * cos(mPitch) * cos(mYaw);
			
			Quaternion newRot = Quaternion::LookRotation(mTarget - eye, Vector3::UP);
			if (Quaternion::Dot(newRot, mTargetRot) < 0.0f)
				newRot = -newRot;
            mTargetRot = newRot;

			mTargetPos = eye;
		}
		else if (mMode == CameraMode::Fly)
		{
			if (isRightMouse)
				UpdateRotation(-dx, -dy);
			UpdateMovement(deltaTime);
		}

		mCurrentPos = VInterpTo(mCurrentPos, mTargetPos, deltaTime, 8.0f);
		mCurrentRot = QInterpTo(mCurrentRot, mTargetRot, deltaTime, 8.0f);

		mTargetCamera.SetPosition(mCurrentPos);
		mTargetCamera.SetRotation(mCurrentRot);
		mTargetCamera.Update();
	}

	void DebugCamera::UpdateOrbit(float dx, float dy)
	{
		mYaw -= dx * mRotateSpeed;
		mPitch -= dy * mRotateSpeed;

		mPitch = std::clamp(mPitch, -Math::HalfPI + 0.1f, Math::HalfPI - 0.1f);
	}

	void DebugCamera::UpdatePan(float dx, float dy)
	{
		Vector3 right = mTargetCamera.GetRightVec();
		Vector3 up = mTargetCamera.GetUpVec();

		mTarget += (-right * dx * mPanSpeed) + (up * dy * mPanSpeed);
	}

	void DebugCamera::UpdateZoom(float dy)
	{
		mRadius *= (1.0f - dy * mZoomSpeed * 0.01f);
		mRadius = Math::Max(mRadius, 0.1f);
	}

	void DebugCamera::UpdateMovement(float dt)
	{
		Vector3 forward = mTargetRot * Vector3::FORWARD;
		Vector3 right = mTargetRot * Vector3::RIGHT;
		Vector3 up = mTargetRot * Vector3::UP;

		Vector3 dir = Vector3::ZERO;

		if (mInput->IsPressKey('W')) dir += forward;
		if (mInput->IsPressKey('S')) dir -= forward;
		if (mInput->IsPressKey('A')) dir -= right;
		if (mInput->IsPressKey('D')) dir += right;
		if (mInput->IsPressKey('E')) dir += up;
		if (mInput->IsPressKey('Q')) dir -= up;

		if (dir.LengthSqr() > 0.0f)
			dir.Normalize();

		float speed = mMoveSpeed;
		if (mInput->IsPressKey(VK_SHIFT))
			speed *= mBoostMultiplier;

		mTargetPos += dir * speed * dt;
	}

	void DebugCamera::UpdateRotation(float dx, float dy)
	{
		mYaw -= dx * mRotateSpeed * 0.5f;
		mPitch -= dy * mRotateSpeed * 0.5f;
		mPitch = std::clamp(mPitch, -Math::HalfPI + 0.05f, Math::HalfPI - 0.05f);

		Quaternion qYaw = Quaternion::GetQuaternionFromAngleAxis(Radian(mYaw), Vector3::UP);
		Quaternion qPitch = Quaternion::GetQuaternionFromAngleAxis(Radian(mPitch), Vector3::RIGHT);
		mTargetRot = qPitch * qYaw;
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
	Quaternion DebugCamera::QInterpTo(const Quaternion& current, const Quaternion& target, float dt, float speed)
	{
		if (speed <= 0.0f) return target;
		float alpha = std::clamp(dt * speed, 0.0f, 1.0f);
		return Quaternion::Slerp(current, target, alpha);
	}
}