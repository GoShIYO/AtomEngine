#pragma once
#include "Runtime/Function/Framework/ECS/World.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "../Component/LadderComponent.h"

class LadderSystem;

using namespace AtomEngine;

class LadderEditor
{
public:
    LadderEditor() = default;

    void SetLadderSystem(LadderSystem* system) { mLadderSystem = system; }

    void RenderUI(World& world, const Camera& camera);

    void RenderVisualization(World& world, const Matrix4x4& viewProj);

    Entity GetSelectedLadder() const { return mSelectedLadder; }

private:
    void RenderLadderList(World& world);

    void RenderLadderProperties(World& world);

    void RenderCreateLadderDialog(World& world);

    void RenderGizmo(World& world, const Camera& camera);

    void DrawCollisionBox(const Vector3& position, const Vector3& halfExtents,
        const Color& color, const Matrix4x4& viewProj);

private:
    LadderSystem* mLadderSystem{ nullptr };
    Entity mSelectedLadder{ entt::null };

    bool mShowCreateDialog{ false };
    LadderCreateInfo mCreateInfo;

    bool mShowCollisionBoxes{ true };

    int mGizmoOperation{ 0 };  // 0: Translate, 1: Scale
    bool mUseSnap{ false };
    float mSnapTranslate{ 1.0f };

    char mSaveFilePath[260] = "Asset/Config/ladders.json";
};
