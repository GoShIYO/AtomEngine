#pragma once
#include "Runtime/Function/Framework/ECS/World.h"
#include "Runtime/Function/Camera/CameraController.h"

class PlayerSystem
{
public:
	void Update(AtomEngine::World& world, const AtomEngine::Camera& camera,float dt);
};

