#pragma once
#include "Runtime/Function/Framework/ECS/World.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "../Component/PlatformComponent.h"
#include "../Component/DynamicVoxelBodyComponent.h"

class PlatformSystem;

using namespace AtomEngine;

enum class GizmoOperation
{
    Translate = 0,
    Rotate = 1,
    Scale = 2
};

class PlatformEditor
{
public:
    PlatformEditor() = default;

    void SetPlatformSystem(PlatformSystem* system) { mPlatformSystem = system; }

    void RenderUI(World& world, const Camera& camera);

    void RenderVisualization(World& world, const Matrix4x4& viewProj);

    Entity GetSelectedPlatform() const { return mSelectedPlatform; }

    void SetSelectedPlatform(Entity entity) { mSelectedPlatform = entity; }

private:
    void RenderPlatformList(World& world);

    void RenderPlatformProperties(World& world);

    void RenderCreatePlatformDialog(World& world);

    void RenderGizmo(World& world, const Camera& camera);
    
    void DrawMotionPath(const PlatformComponent& platform, const Matrix4x4& viewProj);
    
    void DrawCollisionBox(const TransformComponent& transform,
        const DynamicVoxelBodyComponent& body,
        const Color& color,
        const Matrix4x4& viewProj);

private:
    PlatformSystem* mPlatformSystem{ nullptr };
    Entity mSelectedPlatform{ entt::null };

    bool mShowCreateDialog{ false };
    PlatformCreateInfo mCreateInfo;

    bool mShowCollisionBoxes{ true };
    bool mShowMotionPaths{ true };
    float mPathVisualizationSegments{ 32 };

    GizmoOperation mGizmoOperation{ GizmoOperation::Translate };
    bool mUseLocalSpace{ false };
    bool mUseSnap{ false };
    float mSnapTranslate{ 1.0f };
    float mSnapRotate{ 15.0f };
    float mSnapScale{ 0.5f };

    char mSaveFilePath[260] = "Asset/Config/platforms.json";
};
