#pragma once
#include "Runtime/Core/Math/MathInclude.h"

struct VoxelColliderComponent
{
    AtomEngine::Vector3 halfExtents{ 0.5f, 0.5f, 0.5f };
    AtomEngine::Vector3 offset{ 0.0f, 0.0f, 0.0f };
    
    bool enableCollision{ true };
    bool enableSliding{ true };
    bool isGrounded{ false };
    
    float groundCheckDistance{ 0.05f };
    int maxSlideIterations{ 4 };
    
    bool enableStepClimbing{ true };
    bool useAutoStepHeight{ true };
    float maxStepHeight{ 1.0f };
    float stepSearchOvershoot{ 0.1f };
    float minStepDepth{ 0.05f };
    
    bool smoothStepClimbing{ true };
    float stepClimbSpeed{ 15.0f };
    bool isClimbing{ false };
    AtomEngine::Vector3 climbTargetPos{ 0.0f, 0.0f, 0.0f };

    bool useGravity{ true };
    float gravityScale{ 1.0f };
    float verticalVelocity{ 0.0f };
    float maxFallSpeed{ 50.0f };
    
    AtomEngine::Vector3 lastCollisionNormal{ 0.0f, 0.0f, 0.0f };
    bool wasColliding{ false };
};

struct VoxelCollisionEvent
{
    AtomEngine::Entity entity;
    AtomEngine::Vector3 position;
    AtomEngine::Vector3 normal;
    int voxelX, voxelY, voxelZ;
};

struct GroundedStateChangedEvent
{
    AtomEngine::Entity entity;
    bool isGrounded;
};
