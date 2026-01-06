#pragma once
#include "Runtime/Function/Camera/CameraController.h"
#include "Runtime/Function/Framework/ECS/World.h"

namespace AtomEngine
{
	class Input;
}

using namespace AtomEngine;

class InGameCameraController : public CameraController
{
public:
	InGameCameraController(Camera& camera);

	void Update(float dt) override;

	void SetTargetEntity(Entity entity) { mTargetEntity = entity; }
	void SetWorld(World* world) { mWorld = world; }

	void SetDistance(float dist) { mTargetDistance = std::clamp(dist, mMinDistance, mMaxDistance); }
	float GetDistance() const { return mCurrentDistance; }

	void SetDistanceRange(float minDist, float maxDist) { mMinDistance = minDist; mMaxDistance = maxDist; }

	void SetHeight(float height) { mHeightOffset = height; }
	float GetHeight() const { return mHeightOffset; }

	void SetPitch(float pitch) { mTargetPitch = std::clamp(pitch, mMinPitch, mMaxPitch); }
	float GetPitch() const { return mCurrentPitch; }
	void SetPitchRange(float minPitch, float maxPitch) { mMinPitch = minPitch; mMaxPitch = maxPitch; }

	void SetYaw(float yaw) { mTargetYaw = yaw; }
	float GetYaw() const { return mCurrentYaw; }

	void SetRotationSpeed(float speed) { mRotationSpeed = speed; }
	void SetZoomSpeed(float speed) { mZoomSpeed = speed; }
	void SetSmoothSpeed(float speed) { mSmoothSpeed = speed; }

	Vector3 GetTargetPosition() const;

private:
	void UpdateInput(float dt);
	void UpdateCameraPosition(float dt);
	void ApplyToCamera();

private:
	World* mWorld{ nullptr };
	Entity mTargetEntity{ entt::null };
	Input* mInput{ nullptr };

	float mTargetYaw{ 0.0f };
	float mTargetPitch{ 0.4f };
	float mTargetDistance{ 30.0f };

	float mCurrentYaw{ 0.0f };
	float mCurrentPitch{ 0.4f };
	float mCurrentDistance{ 30.0f };
	Vector3 mCurrentLookAt{ 0.0f, 0.0f, 0.0f };

	float mMinDistance{ 15.0f };
	float mMaxDistance{ 50.0f };


	float mMinPitch{ 0.0f };
	float mMaxPitch{ 1.2f };

	float mHeightOffset{ 8.0f };

	float mRotationSpeed{ 2.5f };
	float mPitchSpeed{ 1.0f };
	float mZoomSpeed{ 45.0f };
	float mSmoothSpeed{ 8.0f };

	float mInputYaw{ 0.0f };
	float mInputPitch{ 0.0f };
	float mInputZoom{ 0.0f };
};
