#include "MoveSystem.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include "../Component/VelocityComponent.h"

using namespace AtomEngine;

void MoveSystem::Update(World& world, float deltaTime)
{
	auto view = world.View<TransformComponent, VelocityComponent>();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& v = view.get<VelocityComponent>(entity);

		transform.transition += v.velocity * deltaTime;
	}
}