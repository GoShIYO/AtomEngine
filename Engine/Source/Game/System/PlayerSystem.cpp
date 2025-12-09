#include "PlayerSystem.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include "Runtime/Function/Input/Input.h"

#include "../Tag.h"
#include "../Voxel/VoxelWorld.h"

using namespace AtomEngine;

void PlayerSystem::Update(World& world, const Camera& camera, float dt)
{
	auto view = world.View<TransformComponent, Body, PlayerTag>();
	auto* input = Input::GetInstance();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& body = view.get<Body>(entity);

		auto moveForward = camera.GetForwardVec();
		moveForward.y = 0;
		moveForward.Normalize();
		Vector3 moveRight = Vector3::UP.Cross(moveForward).NormalizedCopy();

		Vector3 move{};

		const auto& joystick = input->GetGamePadStick();
		if (fabs(joystick.leftStickX) > 0.1f) move += moveRight * joystick.leftStickX;
		if (fabs(joystick.leftStickY) > 0.1f) move += moveForward * joystick.leftStickY;

		if (input->IsPressGamePad(LB) || input->IsPressKey('Q'))move.y = 1.0f;
		if (input->IsPressGamePad(RB) || input->IsPressKey('E'))move.y = -1.0f;

		if (input->IsPressKey('W'))move += moveForward;
		if (input->IsPressKey('S'))move += -moveForward;

		if (input->IsPressKey('D'))move += moveRight;
		if (input->IsPressKey('A'))move += -moveRight;

		if (move.LengthSqr() > 0.0001f)
		{
			move.Normalize();

			float angleY = std::atan2(move.x, move.z);

			Quaternion targetRot = Quaternion::GetQuaternionFromAngleAxis(Radian(angleY + Math::PI),Vector3::UP);
			transform.rotation = Slerp(transform.rotation, targetRot, 0.1f, true);
		}
		body.velocity = move * 8.0f;
		transform.transition += body.velocity * dt;
	}
}