#include "LadderEditor.h"
#include "../System/LadderSystem.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include "Runtime/Function/Render/Primitive.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <fstream>
#include <json.hpp>

using namespace AtomEngine;
using json = nlohmann::json;

void LadderEditor::RenderUI(World& world, const Camera& camera)
{
	ImGui::Begin("Ladder Editor");

	if (ImGui::Button("Create Ladder"))
	{
		mShowCreateDialog = true;
		mCreateInfo = LadderCreateInfo{};
		mCreateInfo.position = camera.GetPosition() + camera.GetForwardVec() * 5.0f;
	}

	ImGui::SameLine();
	ImGui::Checkbox("Show Collision Boxes", &mShowCollisionBoxes);

	ImGui::Separator();

	RenderLadderList(world);

	ImGui::Separator();

	RenderLadderProperties(world);

	ImGui::Separator();
	ImGui::Text("Gizmo Settings");
	ImGui::RadioButton("Translate", &mGizmoOperation, 0);
	ImGui::SameLine();
	ImGui::RadioButton("Scale", &mGizmoOperation, 1);
	ImGui::Checkbox("Snap", &mUseSnap);
	if (mUseSnap)
	{
		ImGui::DragFloat("Snap Value", &mSnapTranslate, 0.1f, 0.1f, 10.0f);
	}

	ImGui::Separator();

	ImGui::InputText("File Path", mSaveFilePath, 260);
	if (ImGui::Button("Save"))
	{
		if (mLadderSystem)
		{
			mLadderSystem->SaveToJson(world, mSaveFilePath);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Load"))
	{
		if (mLadderSystem)
		{
			mLadderSystem->LoadFromJson(world, mSaveFilePath);
		}
	}

	ImGui::End();

	if (mShowCreateDialog)
	{
		RenderCreateLadderDialog(world);
	}

	RenderGizmo(world, camera);
}

void LadderEditor::RenderLadderList(World& world)
{
	ImGui::Text("Ladders:");

	auto& registry = world.GetRegistry();
	auto view = registry.view<TransformComponent, LadderComponent>();

	ImGui::BeginChild("LadderList", ImVec2(0, 150), true);

	for (auto entity : view)
	{
		auto& ladder = view.get<LadderComponent>(entity);

		bool isSelected = (mSelectedLadder == entity);
		char label[128];
		snprintf(label, 128, "[%d] %s", ladder.ladderId, ladder.ladderName.c_str());

		if (ImGui::Selectable(label, isSelected))
		{
			mSelectedLadder = entity;
		}
	}

	ImGui::EndChild();

	if (mSelectedLadder != entt::null && registry.valid(mSelectedLadder))
	{
		if (ImGui::Button("Delete Selected"))
		{
			if (mLadderSystem)
			{
				mLadderSystem->DestroyLadder(world, mSelectedLadder);
			}
			mSelectedLadder = entt::null;
		}
	}
}

void LadderEditor::RenderLadderProperties(World& world)
{
	if (mSelectedLadder == entt::null)
	{
		ImGui::Text("No ladder selected");
		return;
	}

	auto& registry = world.GetRegistry();
	if (!registry.valid(mSelectedLadder) || !registry.any_of<LadderComponent>(mSelectedLadder))
	{
		mSelectedLadder = entt::null;
		return;
	}

	auto& transform = registry.get<TransformComponent>(mSelectedLadder);
	auto& ladder = registry.get<LadderComponent>(mSelectedLadder);

	ImGui::Text("Ladder Properties");

	char nameBuffer[64];
	strncpy_s(nameBuffer, ladder.ladderName.c_str(), 63);
	if (ImGui::InputText("Name", nameBuffer, 64))
	{
		ladder.ladderName = nameBuffer;
	}

	float pos[3] = { transform.transition.x, transform.transition.y, transform.transition.z };
	if (ImGui::DragFloat3("Position", pos, 0.1f))
	{
		transform.transition = Vector3(pos[0], pos[1], pos[2]);
	}

	float size[3] = { ladder.halfExtents.x, ladder.halfExtents.y, ladder.halfExtents.z };
	if (ImGui::DragFloat3("Half Extents", size, 0.1f, 0.1f, 100.0f))
	{
		ladder.halfExtents = Vector3(size[0], size[1], size[2]);
	}

	ImGui::DragFloat("Climb Speed", &ladder.climbSpeed, 0.1f, 0.1f, 20.0f);

	ImGui::DragFloat("Top Exit Forward", &ladder.topExitForwardOffset, 0.1f, 0.0f, 5.0f);
	ImGui::DragFloat("Top Exit Up", &ladder.topExitUpOffset, 0.1f, 0.0f, 2.0f);

	ImGui::Checkbox("Enabled", &ladder.isEnabled);
}

void LadderEditor::RenderCreateLadderDialog(World& world)
{
	ImGui::Begin("Create Ladder", &mShowCreateDialog);

	char nameBuffer[64] = "New Ladder";
	strncpy_s(nameBuffer, mCreateInfo.name.c_str(), 63);
	if (ImGui::InputText("Name", nameBuffer, 64))
	{
		mCreateInfo.name = nameBuffer;
	}

	float pos[3] = { mCreateInfo.position.x, mCreateInfo.position.y, mCreateInfo.position.z };
	if (ImGui::DragFloat3("Position", pos, 0.1f))
	{
		mCreateInfo.position = Vector3(pos[0], pos[1], pos[2]);
	}

	float size[3] = { mCreateInfo.halfExtents.x, mCreateInfo.halfExtents.y, mCreateInfo.halfExtents.z };
	if (ImGui::DragFloat3("Half Extents", size, 0.1f, 0.1f, 100.0f))
	{
		mCreateInfo.halfExtents = Vector3(size[0], size[1], size[2]);
	}

	ImGui::DragFloat("Climb Speed", &mCreateInfo.climbSpeed, 0.1f, 0.1f, 20.0f);

	ImGui::Separator();

	if (ImGui::Button("Create"))
	{
		if (mLadderSystem)
		{
			Entity newLadder = mLadderSystem->CreateLadder(world, mCreateInfo);
			mSelectedLadder = newLadder;
		}
		mShowCreateDialog = false;
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel"))
	{
		mShowCreateDialog = false;
	}

	ImGui::End();
}

void LadderEditor::RenderGizmo(World& world, const Camera& camera)
{
	if (mSelectedLadder == entt::null)
	{
		return;
	}

	auto& registry = world.GetRegistry();
	if (!registry.valid(mSelectedLadder) || !registry.any_of<TransformComponent>(mSelectedLadder))
	{
		return;
	}

	auto& transform = registry.get<TransformComponent>(mSelectedLadder);
	auto& ladder = registry.get<LadderComponent>(mSelectedLadder);

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::BeginFrame();

	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

	Matrix4x4 view = camera.GetViewMatrix();
	Matrix4x4 proj = camera.GetProjMatrix();

	Matrix4x4 model = Matrix4x4::IDENTITY;
	model.setTrans(transform.transition);

	float viewMat[16], projMat[16], modelMat[16];
	memcpy(viewMat, view.mat, sizeof(float) * 16);
	memcpy(projMat, proj.mat, sizeof(float) * 16);
	memcpy(modelMat, model.mat, sizeof(float) * 16);

	ImGuizmo::OPERATION op = (mGizmoOperation == 0) ? ImGuizmo::TRANSLATE : ImGuizmo::SCALE;

	float snap[3] = { mSnapTranslate, mSnapTranslate, mSnapTranslate };

	if (ImGuizmo::Manipulate(viewMat, projMat, op, ImGuizmo::WORLD,
		modelMat, nullptr, mUseSnap ? snap : nullptr))
	{
		if (mGizmoOperation == 0)  // Translate
		{
			transform.transition.x = modelMat[12];
			transform.transition.y = modelMat[13];
			transform.transition.z = modelMat[14];
		}
		else  // Scale
		{
			float scaleX = Vector3(modelMat[0], modelMat[1], modelMat[2]).Length();
			float scaleY = Vector3(modelMat[4], modelMat[5], modelMat[6]).Length();
			float scaleZ = Vector3(modelMat[8], modelMat[9], modelMat[10]).Length();
			ladder.halfExtents.x *= scaleX;
			ladder.halfExtents.y *= scaleY;
			ladder.halfExtents.z *= scaleZ;
		}
	}
}

void LadderEditor::RenderVisualization(World& world, const Matrix4x4& viewProj)
{
#ifndef RELEASE
	if (!mShowCollisionBoxes) return;

	auto& registry = world.GetRegistry();
	auto view = registry.view<TransformComponent, LadderComponent>();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& ladder = view.get<LadderComponent>(entity);

		Color color = ladder.isEnabled ? Color(0.2f, 0.8f, 0.2f, 0.5f) : Color(0.5f, 0.5f, 0.5f, 0.3f);

		if (entity == mSelectedLadder)
		{
			color = Color(1.0f, 1.0f, 0.0f, 0.7f);
		}

		DrawCollisionBox(transform.transition, ladder.halfExtents * 2.0f, color, viewProj);
	}
#endif
}

void LadderEditor::DrawCollisionBox(const Vector3& position, const Vector3& halfExtents,
	const Color& color, const Matrix4x4& viewProj)
{
	Primitive::DrawCube(position, halfExtents, color, viewProj);
}
