#include "InGameCameraController.h"
#include "Runtime/Function/Input/Input.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include <cmath>
#include <algorithm>

InGameCameraController::InGameCameraController(Camera& camera)
	: CameraController(camera)
{
	mInput = Input::GetInstance();
}

void InGameCameraController::Update(float dt)
{
	UpdateInput(dt);

	UpdateCameraPosition(dt);

	ApplyToCamera();

	mTargetCamera.Update();
}

void InGameCameraController::UpdateInput(float dt)
{
	mInputYaw = 0.0f;
	mInputPitch = 0.0f;
	mInputZoom = 0.0f;

	if (!mInput) return;

	if (mInput->IsContactGamePad())
	{
		const PadStick& stick = mInput->GetGamePadStick();

		if (std::abs(stick.rightStickX) > 0.15f)
		{
			mInputYaw = -stick.rightStickX;
		}

		if (std::abs(stick.rightStickY) > 0.15f)
		{
			mInputPitch = -stick.rightStickY;
		}

		float zoomInput = stick.leftTrigger - stick.rightTrigger;
		if (std::abs(zoomInput) > 0.1f)
		{
			mInputZoom = -zoomInput;
		}

		if (mInput->IsPressGamePad(GamePadButton::LB))
		{
			mInputZoom = -1.0f;
		}
		if (mInput->IsPressGamePad(GamePadButton::RB))
		{
			mInputZoom = 1.0f;
		}
	}

	if (mInput->IsPressKey(VK_LEFT))
	{
		mInputYaw = 1.0f;
	}
	if (mInput->IsPressKey(VK_RIGHT))
	{
		mInputYaw = -1.0f;
	}

	if (mInput->IsPressKey(VK_UP))
	{
		mInputPitch = -1.0f;
	}
	if (mInput->IsPressKey(VK_DOWN))
	{
		mInputPitch = 1.0f;
	}

	if (mInput->IsPressKey(VK_PRIOR))
	{
		mInputZoom = -1.0f;
	}
	if (mInput->IsPressKey(VK_NEXT))
	{
		mInputZoom = 1.0f;
	}

	mTargetYaw += mInputYaw * mRotationSpeed * dt;

	while (mTargetYaw > Math::PI) mTargetYaw -= Math::TwoPI;
	while (mTargetYaw < -Math::PI) mTargetYaw += Math::TwoPI;

	mTargetPitch += mInputPitch * mPitchSpeed * dt;
	mTargetPitch = std::clamp(mTargetPitch, mMinPitch, mMaxPitch);

	mTargetDistance += mInputZoom * mZoomSpeed * dt;
	mTargetDistance = std::clamp(mTargetDistance, mMinDistance, mMaxDistance);
}

void InGameCameraController::UpdateCameraPosition(float dt)
{
	float lerpFactor = 1.0f - std::exp(-mSmoothSpeed * dt);

	float yawDiff = mTargetYaw - mCurrentYaw;

	while (yawDiff > Math::PI) yawDiff -= Math::TwoPI;
	while (yawDiff < -Math::PI) yawDiff += Math::TwoPI;

	mCurrentYaw = mCurrentYaw + yawDiff * lerpFactor;

	while (mCurrentYaw > Math::PI) mCurrentYaw -= Math::TwoPI;
	while (mCurrentYaw < -Math::PI) mCurrentYaw += Math::TwoPI;

	mCurrentPitch = mCurrentPitch + (mTargetPitch - mCurrentPitch) * lerpFactor;
	mCurrentDistance = mCurrentDistance + (mTargetDistance - mCurrentDistance) * lerpFactor;

	Vector3 targetPos = GetTargetPosition();

	mCurrentLookAt = mCurrentLookAt + (targetPos - mCurrentLookAt) * lerpFactor;
}

Vector3 InGameCameraController::GetTargetPosition() const
{
	if (!mWorld || mTargetEntity == entt::null)
	{
		return Vector3::ZERO;
	}

	if (!mWorld->HasComponent<TransformComponent>(mTargetEntity))
	{
		return Vector3::ZERO;
	}

	auto& transform = mWorld->GetComponent<TransformComponent>(mTargetEntity);

	return transform.transition + Vector3(0.0f, mHeightOffset, 0.0f);
}

void InGameCameraController::ApplyToCamera()
{
	float cosPitch = std::cos(mCurrentPitch);
	float sinPitch = std::sin(mCurrentPitch);
	float cosYaw = std::cos(mCurrentYaw);
	float sinYaw = std::sin(mCurrentYaw);

	Vector3 offset;
	offset.x = mCurrentDistance * cosPitch * sinYaw;
	offset.y = mCurrentDistance * sinPitch;
	offset.z = -mCurrentDistance * cosPitch * cosYaw;

	Vector3 cameraPos = mCurrentLookAt + offset;

	mTargetCamera.SetLookAt(cameraPos, mCurrentLookAt, Vector3::UP);
}
