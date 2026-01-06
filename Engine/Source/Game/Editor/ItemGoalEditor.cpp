#include "ItemGoalEditor.h"
#include "../System/ItemSystem.h"
#include "../System/GoalSystem.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include "Runtime/Function/Render/Primitive.h"
#include "Runtime/Core/Utility/Utility.h"
#include "../Tag.h"
#include <imgui.h>
#include <cmath>

using namespace AtomEngine;

void ItemGoalEditor::RenderUI(World& world, const Camera& camera)
{
	ImGui::Begin("Item & Goal Editor");

	// 表示設定
	if (ImGui::CollapsingHeader("Display Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Show Item Markers", &mShowItemMarkers);
		ImGui::Checkbox("Show Goal Markers", &mShowGoalMarkers);
	}

	ImGui::Separator();

	// Item セクション
	RenderItemSection(world, camera);

	ImGui::Separator();

	// Goal セクション
	RenderGoalSection(world, camera);

	ImGui::End();

	// ダイアログ
	if (mShowCreateItemDialog)
	{
		RenderCreateItemDialog(world);
	}

	if (mShowCreateGoalDialog)
	{
		RenderCreateGoalDialog(world);
	}
}

void ItemGoalEditor::RenderItemSection(World& world, const Camera& camera)
{
	if (ImGui::CollapsingHeader("Items", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 統計情報
		if (mItemSystem)
		{
			ImGui::Text("Total Items: %d", mItemSystem->GetTotalCount());
			ImGui::Text("Collected: %d", mItemSystem->GetCollectedCount());
		}

		ImGui::Separator();

		// 新規作成ボタン
		if (ImGui::Button("Create Item"))
		{
			mShowCreateItemDialog = true;
			mItemCreateInfo = ItemCreateInfo{};
			mItemCreateInfo.position = camera.GetPosition() + camera.GetForwardVec() * 10.0f;
		}

		ImGui::SameLine();

		// 保存/読み込み
		if (ImGui::Button("Save Items"))
		{
			if (mItemSystem)
			{
				if (mItemSystem->SaveToJson(world, mItemSavePath))
				{
					ImGui::OpenPopup("ItemSaveSuccess");
				}
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Load Items"))
		{
			if (mItemSystem)
			{
				mItemSystem->LoadFromJson(world, mItemSavePath);
			}
		}

		ImGui::InputText("Item Save Path", mItemSavePath, sizeof(mItemSavePath));

		// ポップアップ
		if (ImGui::BeginPopup("ItemSaveSuccess"))
		{
			ImGui::Text("Items saved successfully!");
			if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		ImGui::Separator();

		// Itemリスト
		ImGui::Text("Item List:");
		ImGui::BeginChild("ItemList", ImVec2(0, 150), true);

		auto& registry = world.GetRegistry();
		auto view = registry.view<TransformComponent, ItemComponent, ItemTag>();

		int index = 0;
		for (auto entity : view)
		{
			auto& item = view.get<ItemComponent>(entity);

			bool isSelected = (mSelectedItem == entity);
			char label[64];
			snprintf(label, sizeof(label), "Item_%d [ID: %d]%s", 
				index, item.itemId, item.isPickedUp ? " (Picked)" : "");

			if (ImGui::Selectable(label, isSelected))
			{
				mSelectedItem = entity;
			}

			// 右クリックメニュー
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Delete"))
				{
					if (mItemSystem)
					{
						mItemSystem->DestroyItem(world, entity);
						if (mSelectedItem == entity)
						{
							mSelectedItem = entt::null;
						}
					}
				}
				ImGui::EndPopup();
			}

			index++;
		}

		ImGui::EndChild();

		// 選択中Itemのプロパティ
		if (mSelectedItem != entt::null && registry.valid(mSelectedItem) &&
			registry.any_of<ItemComponent>(mSelectedItem))
		{
			ImGui::Separator();
			ImGui::Text("Item Properties");

			auto& transform = registry.get<TransformComponent>(mSelectedItem);
			auto& item = registry.get<ItemComponent>(mSelectedItem);

			ImGui::DragFloat3("Position", transform.transition.ptr(), 0.1f);
			
			if (ImGui::DragFloat("Scale", &transform.scale.x, 0.1f, 0.1f, 10.0f))
			{
				transform.scale.y = transform.scale.x;
				transform.scale.z = transform.scale.x;
			}

			ImGui::DragFloat("Rotation Speed", &item.rotationSpeed, 0.1f, 0.0f, 10.0f);
			ImGui::DragFloat("Bob Speed", &item.bobSpeed, 0.1f, 0.0f, 10.0f);
			ImGui::DragFloat("Bob Amount", &item.bobAmount, 0.01f, 0.0f, 2.0f);

			// 位置をbaseYに同期
			if (ImGui::Button("Set Base Y to Current"))
			{
				item.baseY = transform.transition.y;
			}
		}
	}
}

void ItemGoalEditor::RenderGoalSection(World& world, const Camera& camera)
{
	if (ImGui::CollapsingHeader("Goals", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 状態表示
		if (mGoalSystem)
		{
			ImGui::Text("Goal Reached: %s", mGoalSystem->IsGoalReached() ? "Yes" : "No");
		}

		ImGui::Separator();

		// 新規作成ボタン
		if (ImGui::Button("Create Goal"))
		{
			mShowCreateGoalDialog = true;
			mGoalCreateInfo = GoalCreateInfo{};
			mGoalCreateInfo.position = camera.GetPosition() + camera.GetForwardVec() * 10.0f;
		}

		ImGui::SameLine();

		// 保存/読み込み
		if (ImGui::Button("Save Goals"))
		{
			if (mGoalSystem)
			{
				if (mGoalSystem->SaveToJson(world, mGoalSavePath))
				{
					ImGui::OpenPopup("GoalSaveSuccess");
				}
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Load Goals"))
		{
			if (mGoalSystem)
			{
				mGoalSystem->LoadFromJson(world, mGoalSavePath);
			}
		}

		ImGui::InputText("Goal Save Path", mGoalSavePath, sizeof(mGoalSavePath));

		// ポップアップ
		if (ImGui::BeginPopup("GoalSaveSuccess"))
		{
			ImGui::Text("Goals saved successfully!");
			if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		ImGui::Separator();

		// Goalリスト
		ImGui::Text("Goal List:");
		ImGui::BeginChild("GoalList", ImVec2(0, 100), true);

		auto& registry = world.GetRegistry();
		auto view = registry.view<TransformComponent, GoalComponent, GoalTag>();

		int index = 0;
		for (auto entity : view)
		{
			auto& goal = view.get<GoalComponent>(entity);

			bool isSelected = (mSelectedGoal == entity);
			char label[64];
			snprintf(label, sizeof(label), "Goal_%d%s", 
				index, goal.isReached ? " (Reached)" : "");

			if (ImGui::Selectable(label, isSelected))
			{
				mSelectedGoal = entity;
			}

			// 右クリックメニュー
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Delete"))
				{
					if (mGoalSystem)
					{
						mGoalSystem->DestroyGoal(world, entity);
						if (mSelectedGoal == entity)
						{
							mSelectedGoal = entt::null;
						}
					}
				}
				ImGui::EndPopup();
			}

			index++;
		}

		ImGui::EndChild();

		// 選択中Goalのプロパティ
		if (mSelectedGoal != entt::null && registry.valid(mSelectedGoal) &&
			registry.any_of<GoalComponent>(mSelectedGoal))
		{
			ImGui::Separator();
			ImGui::Text("Goal Properties");

			auto& transform = registry.get<TransformComponent>(mSelectedGoal);
			auto& goal = registry.get<GoalComponent>(mSelectedGoal);

			ImGui::DragFloat3("Position", transform.transition.ptr(), 0.1f);
			
			if (ImGui::DragFloat("Scale", &transform.scale.x, 0.1f, 0.1f, 10.0f))
			{
				transform.scale.y = transform.scale.x;
				transform.scale.z = transform.scale.x;
			}

			ImGui::DragFloat("Trigger Radius", &goal.triggerRadius, 0.1f, 0.5f, 20.0f);

			// リセットボタン
			if (ImGui::Button("Reset Goal State"))
			{
				goal.isReached = false;
				if (mGoalSystem)
				{
					mGoalSystem->Reset();
				}
			}
		}
	}
}

void ItemGoalEditor::RenderCreateItemDialog(World& world)
{
	ImGui::SetNextWindowSize(ImVec2(350, 300), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Create Item", &mShowCreateItemDialog))
	{
		ImGui::DragFloat3("Position", mItemCreateInfo.position.ptr(), 0.1f);
		ImGui::DragFloat("Scale", &mItemCreateInfo.scale, 0.1f, 0.1f, 10.0f);

		char modelPathBuf[260];
		std::string modelPath = WStringToUTF8(mItemCreateInfo.modelPath);
		strncpy_s(modelPathBuf, modelPath.c_str(), sizeof(modelPathBuf));
		if (ImGui::InputText("Model Path", modelPathBuf, sizeof(modelPathBuf)))
		{
			mItemCreateInfo.modelPath = UTF8ToWString(modelPathBuf);
		}

		char nameBuf[64];
		strncpy_s(nameBuf, mItemCreateInfo.name.c_str(), sizeof(nameBuf));
		if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
		{
			mItemCreateInfo.name = nameBuf;
		}

		ImGui::Separator();

		if (ImGui::Button("Create"))
		{
			if (mItemSystem)
			{
				mSelectedItem = mItemSystem->CreateItem(world, mItemCreateInfo);
			}
			mShowCreateItemDialog = false;
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel"))
		{
			mShowCreateItemDialog = false;
		}
	}
	ImGui::End();
}

void ItemGoalEditor::RenderCreateGoalDialog(World& world)
{
	ImGui::SetNextWindowSize(ImVec2(350, 280), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Create Goal", &mShowCreateGoalDialog))
	{
		ImGui::DragFloat3("Position", mGoalCreateInfo.position.ptr(), 0.1f);
		ImGui::DragFloat("Scale", &mGoalCreateInfo.scale, 0.1f, 0.1f, 10.0f);
		ImGui::DragFloat("Trigger Radius", &mGoalCreateInfo.triggerRadius, 0.1f, 0.5f, 20.0f);

		char modelPathBuf[260];
		std::string modelPath = WStringToUTF8(mGoalCreateInfo.modelPath);
		strncpy_s(modelPathBuf, modelPath.c_str(), sizeof(modelPathBuf));
		if (ImGui::InputText("Model Path", modelPathBuf, sizeof(modelPathBuf)))
		{
			mGoalCreateInfo.modelPath = UTF8ToWString(modelPathBuf);
		}

		char nameBuf[64];
		strncpy_s(nameBuf, mGoalCreateInfo.name.c_str(), sizeof(nameBuf));
		if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
		{
			mGoalCreateInfo.name = nameBuf;
		}

		ImGui::Separator();

		if (ImGui::Button("Create"))
		{
			if (mGoalSystem)
			{
				mSelectedGoal = mGoalSystem->CreateGoal(world, mGoalCreateInfo);
			}
			mShowCreateGoalDialog = false;
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel"))
		{
			mShowCreateGoalDialog = false;
		}
	}
	ImGui::End();
}

void ItemGoalEditor::RenderVisualization(World& world, const Matrix4x4& viewProj)
{
	if (mShowItemMarkers)
	{
		DrawItemMarkers(world, viewProj);
	}

	if (mShowGoalMarkers)
	{
		DrawGoalMarkers(world, viewProj);
	}
}

void ItemGoalEditor::DrawItemMarkers(World& world, const Matrix4x4& viewProj)
{
	auto& registry = world.GetRegistry();
	auto view = registry.view<TransformComponent, ItemComponent, ItemTag>();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& item = view.get<ItemComponent>(entity);

		// 色決定
		Color markerColor = item.isPickedUp ? 
			Color(0.5f, 0.5f, 0.5f, 0.5f) :  // 取得済み: グレー
			Color(1.0f, 0.84f, 0.0f, 1.0f);   // 未取得: 金色

		if (entity == mSelectedItem)
		{
			markerColor = Color(0.0f, 1.0f, 0.0f, 1.0f);  // 選択中: 緑
		}

		// マーカー描画
		float size = transform.scale.x * 0.5f;
		Primitive::DrawCube(transform.transition, Vector3(size, size, size), markerColor, viewProj);

		// 垂直線
		Vector3 lineStart = transform.transition;
		Vector3 lineEnd = transform.transition + Vector3(0, size * 2, 0);
		Primitive::DrawLine(lineStart, lineEnd, markerColor, viewProj);
	}
}

void ItemGoalEditor::DrawGoalMarkers(World& world, const Matrix4x4& viewProj)
{
	auto& registry = world.GetRegistry();
	auto view = registry.view<TransformComponent, GoalComponent, GoalTag>();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& goal = view.get<GoalComponent>(entity);

		// 色決定
		Color markerColor = goal.isReached ?
			Color(0.5f, 0.5f, 0.5f, 0.5f) :  // 到達済み: グレー
			Color(0.0f, 0.8f, 1.0f, 1.0f);   // 未到達: シアン

		if (entity == mSelectedGoal)
		{
			markerColor = Color(1.0f, 0.0f, 1.0f, 1.0f);  // 選択中: マゼンタ
		}

		// ゴールマーカー
		float size = transform.scale.x;
		Primitive::DrawCube(transform.transition, Vector3(size, size * 2, size), markerColor, viewProj);

		// トリガー範囲（円として近似）
		Color rangeColor = markerColor;
		rangeColor.a = 0.3f;

		int segments = 16;
		float angleStep = 6.28318530718f / segments;
		for (int i = 0; i < segments; ++i)
		{
			float angle1 = i * angleStep;
			float angle2 = (i + 1) * angleStep;

			Vector3 p1 = transform.transition + Vector3(
				goal.triggerRadius * std::cos(angle1),
				0.1f,
				goal.triggerRadius * std::sin(angle1)
			);
			Vector3 p2 = transform.transition + Vector3(
				goal.triggerRadius * std::cos(angle2),
				0.1f,
				goal.triggerRadius * std::sin(angle2)
			);

			Primitive::DrawLine(p1, p2, rangeColor, viewProj);
		}
	}
}
