#include "PlayerSystem.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include "Runtime/Function/Framework/Component/MeshComponent.h"

#include "Runtime/Function/Input/Input.h"

#include "../Component/VelocityComponent.h"
#include "../Component/VoxelColliderComponent.h"

#include "../Tag.h"
#include "../Voxel/VoxelWorld.h"
#include "../System/SoundManaged.h"

using namespace AtomEngine;

namespace
{
	constexpr float kMoveSpeed = 12.0f;
}

void PlayerSystem::Update(World& world, const Camera& camera, float dt)
{
	auto view = world.View<TransformComponent, VelocityComponent, MeshComponent, VoxelColliderComponent, PlayerTag>();
	auto* input = Input::GetInstance();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& v = view.get<VelocityComponent>(entity);
		auto& mesh = view.get<MeshComponent>(entity);
		auto& voxelCollider = view.get<VoxelColliderComponent>(entity);

		auto moveForward = camera.GetForwardVec();
		moveForward.y = 0;
		moveForward.Normalize();
		Vector3 moveRight = Vector3::UP.Cross(moveForward).NormalizedCopy();

		Vector3 move{};

		const auto& joystick = input->GetGamePadStick();
		if (fabs(joystick.leftStickX) > 0.1f) move += moveRight * joystick.leftStickX;
		if (fabs(joystick.leftStickY) > 0.1f) move += moveForward * joystick.leftStickY;

		if (input->IsPressKey('W'))move += moveForward;
		if (input->IsPressKey('S'))move += -moveForward;

		if (input->IsPressKey('D'))move += moveRight;
		if (input->IsPressKey('A'))move += -moveRight;

		bool isFall = !voxelCollider.isGrounded;
		bool isPlaySe = Audio::GetInstance()->IsPlaying(Sound::gSoundMap["moveSe"]);

		if (move.LengthSqr() > 0.0001f)
		{
			move.Normalize();

			float angleY = std::atan2(move.x, move.z);

			Quaternion targetRot = Quaternion::GetQuaternionFromAngleAxis(Radian(angleY + Math::PI), Vector3::UP);
			transform.rotation = Slerp(transform.rotation, targetRot, 0.1f, true);
			mesh.PlayAnimation(2, true);
			if (!isPlaySe)
				Audio::GetInstance()->Play(Sound::gSoundMap["moveSe"]);
		}
		else
		{
			mesh.PlayAnimation(1, true);
			if (isPlaySe)
				Audio::GetInstance()->Stop(Sound::gSoundMap["moveSe"]);
		}
		if (isFall)
		{
			mesh.PlayAnimation(0, false);
			if (isPlaySe)
				Audio::GetInstance()->Stop(Sound::gSoundMap["moveSe"]);
		}
		v.velocity = move * kMoveSpeed;
	}
}