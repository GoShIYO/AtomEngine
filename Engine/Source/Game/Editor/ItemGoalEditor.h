#pragma once
#include "Runtime/Function/Framework/ECS/World.h"
#include "Runtime/Function/Camera/CameraBase.h"
#include "../Component/ItemComponent.h"
#include "../Component/GoalComponent.h"

class ItemSystem;
class GoalSystem;

using namespace AtomEngine;

class ItemGoalEditor
{
public:
	ItemGoalEditor() = default;

	void SetItemSystem(ItemSystem* system) { mItemSystem = system; }
	void SetGoalSystem(GoalSystem* system) { mGoalSystem = system; }

	void RenderUI(World& world, const Camera& camera);

	void RenderVisualization(World& world, const Matrix4x4& viewProj);

private:
	void RenderItemSection(World& world, const Camera& camera);
	void RenderGoalSection(World& world, const Camera& camera);
	void RenderCreateItemDialog(World& world);
	void RenderCreateGoalDialog(World& world);

	void DrawItemMarkers(World& world, const Matrix4x4& viewProj);
	void DrawGoalMarkers(World& world, const Matrix4x4& viewProj);

private:
	ItemSystem* mItemSystem{ nullptr };
	GoalSystem* mGoalSystem{ nullptr };

	// Item編集
	Entity mSelectedItem{ entt::null };
	bool mShowCreateItemDialog{ false };
	ItemCreateInfo mItemCreateInfo;

	// Goal編集
	Entity mSelectedGoal{ entt::null };
	bool mShowCreateGoalDialog{ false };
	GoalCreateInfo mGoalCreateInfo;

	// 表示設定
	bool mShowItemMarkers{ true };
	bool mShowGoalMarkers{ true };

	// 保存パス
	char mItemSavePath[260] = "Asset/Config/items.json";
	char mGoalSavePath[260] = "Asset/Config/goals.json";
};
