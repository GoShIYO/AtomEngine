#include "PlatformEditor.h"
#include "../System/PlatformSystem.h"
#include "Runtime/Function/Framework/Component/TransformComponent.h"
#include "Runtime/Function/Framework/Component/MeshComponent.h"
#include "Runtime/Function/Framework/Component/MaterialComponent.h"
#include "Runtime/Function/Render/Primitive.h"
#include "Runtime/Resource/AssetManager.h"
#include "Runtime/Core/Utility/Utility.h"
#include <imgui.h>
#include <ImGuizmo.h>
#include <json.hpp>
#include <fstream>
#include <filesystem>
#include <cmath>

using namespace AtomEngine;
using json = nlohmann::json;

static json Vec3ToJson(const Vector3& v)
{
	return json::array({ v.x, v.y, v.z });
}

static Vector3 JsonToVec3(const json& j)
{
	if (j.is_array() && j.size() >= 3)
		return Vector3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
	return Vector3::ZERO;
}

static json QuatToJson(const Quaternion& q)
{
	return json::array({ q.x, q.y, q.z, q.w });
}

static Quaternion JsonToQuat(const json& j)
{
	if (j.is_array() && j.size() >= 4)
		return Quaternion(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>());
	return Quaternion::IDENTITY;
}

void PlatformEditor::RenderUI(World& world, const Camera& camera)
{
	ImGui::Begin("Platform Editor");

	if (ImGui::CollapsingHeader("Display Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Show Collision Boxes", &mShowCollisionBoxes);
		ImGui::Checkbox("Show Motion Paths", &mShowMotionPaths);
		ImGui::SliderFloat("Path Segments", &mPathVisualizationSegments, 8.0f, 64.0f);
	}

	if (ImGui::CollapsingHeader("Gizmo Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const char* opNames[] = { "Translate", "Rotate", "Scale" };
		int opIndex = static_cast<int>(mGizmoOperation);
		if (ImGui::Combo("Operation", &opIndex, opNames, IM_ARRAYSIZE(opNames)))
		{
			mGizmoOperation = static_cast<GizmoOperation>(opIndex);
		}

		ImGui::Checkbox("Local Space", &mUseLocalSpace);
		ImGui::Checkbox("Use Snap", &mUseSnap);
		if (mUseSnap)
		{
			ImGui::DragFloat("Snap Translate", &mSnapTranslate, 0.1f, 0.1f, 10.0f);
			ImGui::DragFloat("Snap Rotate", &mSnapRotate, 1.0f, 1.0f, 90.0f);
			ImGui::DragFloat("Snap Scale", &mSnapScale, 0.1f, 0.1f, 5.0f);
		}
	}

	ImGui::Separator();

	if (ImGui::Button("Create New Platform"))
	{
		mShowCreateDialog = true;
		mCreateInfo = PlatformCreateInfo{};
	}

	ImGui::Separator();

	RenderPlatformList(world);

	ImGui::Separator();

	RenderPlatformProperties(world);

	ImGui::Separator();

	if (ImGui::CollapsingHeader("Save / Load", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::InputText("File Path", mSaveFilePath, sizeof(mSaveFilePath));

		if (ImGui::Button("Save Platforms"))
		{
			if (mPlatformSystem && mPlatformSystem->SaveToJson(world, mSaveFilePath))
			{
				ImGui::OpenPopup("SaveSuccess");
			}
			else
			{
				ImGui::OpenPopup("SaveError");
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Load Platforms"))
		{
			if (mPlatformSystem && mPlatformSystem->LoadFromJson(world, mSaveFilePath))
			{
				ImGui::OpenPopup("LoadSuccess");
			}
			else
			{
				ImGui::OpenPopup("LoadError");
			}
		}

		if (ImGui::BeginPopup("SaveSuccess"))
		{
			ImGui::Text("Platforms saved successfully!");
			if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopup("SaveError"))
		{
			ImGui::Text("Failed to save platforms.");
			if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopup("LoadSuccess"))
		{
			ImGui::Text("Platforms loaded successfully!");
			if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopup("LoadError"))
		{
			ImGui::Text("Failed to load platforms.");
			if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
	}

	ImGui::End();

	if (mShowCreateDialog)
	{
		RenderCreatePlatformDialog(world);
	}

	RenderGizmo(world, camera);
}

void PlatformEditor::RenderPlatformList(World& world)
{
	ImGui::Text("Platforms:");

	auto& registry = world.GetRegistry();
	auto view = registry.view<TransformComponent, PlatformComponent, NameComponent>();

	int index = 0;
	for (auto entity : view)
	{
		auto& name = view.get<NameComponent>(entity);
		auto& platform = view.get<PlatformComponent>(entity);

		bool isSelected = (entity == mSelectedPlatform);

		ImGui::PushID(index);

		std::string displayName = name.value + " [" + std::to_string(platform.platformId) + "]";

		if (ImGui::Selectable(displayName.c_str(), isSelected))
		{
			if (mSelectedPlatform != entt::null && registry.valid(mSelectedPlatform))
			{
				if (registry.any_of<PlatformComponent>(mSelectedPlatform))
				{
					registry.get<PlatformComponent>(mSelectedPlatform).isSelected = false;
				}
			}

			mSelectedPlatform = entity;
			platform.isSelected = true;
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete"))
			{
				if (mPlatformSystem)
				{
					mPlatformSystem->DestroyPlatform(world, entity);
					if (mSelectedPlatform == entity)
					{
						mSelectedPlatform = entt::null;
					}
				}
			}
			if (ImGui::MenuItem("Duplicate"))
			{
				if (mPlatformSystem)
				{
					auto& transform = view.get<TransformComponent>(entity);
					PlatformCreateInfo info;
					info.position = transform.transition + Vector3(5.0f, 0.0f, 0.0f);
					info.size = platform.platformSize;
					info.motionType = platform.motionType;
					info.modelPath = platform.modelPath;
					info.name = platform.platformName + "_copy";
					mPlatformSystem->CreatePlatform(world, info);
				}
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();
		index++;
	}
}

void PlatformEditor::RenderPlatformProperties(World& world)
{
	if (mSelectedPlatform == entt::null)
	{
		ImGui::Text("No platform selected");
		return;
	}

	auto& registry = world.GetRegistry();
	if (!registry.valid(mSelectedPlatform))
	{
		mSelectedPlatform = entt::null;
		return;
	}

	if (!registry.any_of<PlatformComponent>(mSelectedPlatform))
	{
		ImGui::Text("Selected entity is not a platform");
		return;
	}

	auto& transform = registry.get<TransformComponent>(mSelectedPlatform);
	auto& platform = registry.get<PlatformComponent>(mSelectedPlatform);
	auto& body = registry.get<DynamicVoxelBodyComponent>(mSelectedPlatform);

	ImGui::Text("Platform Properties");

	char nameBuf[128];
	strncpy_s(nameBuf, platform.platformName.c_str(), sizeof(nameBuf));
	if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
	{
		platform.platformName = nameBuf;
	}

	// Transform
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3("Position", transform.transition.ptr(), 0.1f);

		Vector3 eulerDegrees;
		float yaw = std::atan2(2.0f * (transform.rotation.w * transform.rotation.y + transform.rotation.x * transform.rotation.z),
			1.0f - 2.0f * (transform.rotation.y * transform.rotation.y + transform.rotation.z * transform.rotation.z));
		eulerDegrees.y = yaw * 57.2957795f;
		if (ImGui::DragFloat("Rotation Y", &eulerDegrees.y, 1.0f, -180.0f, 180.0f))
		{
			transform.rotation = Quaternion::GetQuaternionFromAngleAxis(Radian(eulerDegrees.y * 0.0174533f), Vector3::UP);
		}
	}

	if (ImGui::CollapsingHeader("Size & Model", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::DragFloat3("Platform Size", platform.platformSize.ptr(), 0.1f, 0.1f, 100.0f))
		{
			// 更新Transform的scale
			transform.scale = platform.platformSize;
			// 更新碰撞体
			body.halfExtentsInVoxels = platform.platformSize * 0.5f / body.voxelSize;
		}

		// 模型路径
		char modelPathBuf[260];
		std::string modelPath = WStringToUTF8(platform.modelPath);
		strncpy_s(modelPathBuf, modelPath.c_str(), sizeof(modelPathBuf));
		if (ImGui::InputText("Model Path", modelPathBuf, sizeof(modelPathBuf)))
		{
			platform.modelPath = UTF8ToWString(modelPathBuf);
			// 重新加载模型
			if (!platform.modelPath.empty())
			{
				auto model = AssetManager::LoadModel(platform.modelPath.c_str());
				if (model)
				{
					// 更新或添加MeshComponent和MaterialComponent
					if (registry.any_of<MeshComponent>(mSelectedPlatform))
					{
						registry.remove<MeshComponent>(mSelectedPlatform);
					}
					if (registry.any_of<MaterialComponent>(mSelectedPlatform))
					{
						registry.remove<MaterialComponent>(mSelectedPlatform);
					}
					registry.emplace<MaterialComponent>(mSelectedPlatform, model);
					registry.emplace<MeshComponent>(mSelectedPlatform, model);
				}
			}
		}
		
		// 材质属性编辑（如果有MaterialComponent）
		if (registry.any_of<MaterialComponent>(mSelectedPlatform))
		{
			auto& material = registry.get<MaterialComponent>(mSelectedPlatform);
			if (!material.mMaterials.empty())
			{
				ImGui::Separator();
				ImGui::Text("Material");
				auto& mat = material.mMaterials[0];
				ImGui::ColorEdit3("Base Color", mat.mBaseColor.ptr());
				ImGui::DragFloat("Metallic", &mat.mMetallic, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Roughness", &mat.mRoughness, 0.01f, 0.0f, 1.0f);
			}
		}
	}

	if (ImGui::CollapsingHeader("Motion", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const char* motionTypes[] = { "Static", "Linear", "Circular", "Custom" };
		int currentType = static_cast<int>(platform.motionType);
		if (ImGui::Combo("Motion Type", &currentType, motionTypes, IM_ARRAYSIZE(motionTypes)))
		{
			platform.motionType = static_cast<PlatformMotionType>(currentType);
			platform.currentTime = 0.0f;
			platform.direction = 1.0f;
		}

		ImGui::Checkbox("Is Moving", &platform.isMoving);

		if (platform.motionType == PlatformMotionType::Linear)
		{
			ImGui::Separator();
			ImGui::Text("Linear Motion Settings");
			ImGui::DragFloat3("Start Position", platform.startPosition.ptr(), 0.1f);
			ImGui::DragFloat3("End Position", platform.endPosition.ptr(), 0.1f);
			ImGui::DragFloat("Move Speed", &platform.moveSpeed, 0.1f, 0.1f, 50.0f);
			ImGui::Checkbox("Ping Pong", &platform.pingPong);

			if (ImGui::Button("Set Current as Start"))
			{
				platform.startPosition = transform.transition;
			}
			ImGui::SameLine();
			if (ImGui::Button("Set Current as End"))
			{
				platform.endPosition = transform.transition;
			}
		}
		else if (platform.motionType == PlatformMotionType::Circular)
		{
			ImGui::Separator();
			ImGui::Text("Circular Motion Settings");
			ImGui::DragFloat3("Circle Center", platform.circleCenter.ptr(), 0.1f);
			ImGui::DragFloat("Radius", &platform.circleRadius, 0.1f, 0.1f, 100.0f);
			ImGui::DragFloat("Rotation Speed", &platform.rotationSpeed, 0.01f, -10.0f, 10.0f);

			const char* axisNames[] = { "Y Axis (XZ Plane)", "X Axis (YZ Plane)", "Z Axis (XY Plane)" };
			int axisIndex = 0;
			if (std::abs(platform.rotationAxis.y) > 0.9f) axisIndex = 0;
			else if (std::abs(platform.rotationAxis.x) > 0.9f) axisIndex = 1;
			else if (std::abs(platform.rotationAxis.z) > 0.9f) axisIndex = 2;

			if (ImGui::Combo("Rotation Axis", &axisIndex, axisNames, IM_ARRAYSIZE(axisNames)))
			{
				switch (axisIndex)
				{
				case 0: platform.rotationAxis = Vector3(0, 1, 0); break;
				case 1: platform.rotationAxis = Vector3(1, 0, 0); break;
				case 2: platform.rotationAxis = Vector3(0, 0, 1); break;
				}
			}

			if (ImGui::Button("Set Current as Center"))
			{
				platform.circleCenter = transform.transition;
			}
		}
	}

	if (ImGui::CollapsingHeader("Self Rotation"))
	{
		ImGui::Checkbox("Enable Self Rotation", &platform.enableRotation);
		if (platform.enableRotation)
		{
			ImGui::DragFloat("Self Rotation Speed", &platform.selfRotationSpeed, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat3("Self Rotation Axis", platform.selfRotationAxis.ptr(), 0.1f);
			if (platform.selfRotationAxis.LengthSqr() > 0.001f)
			{
				platform.selfRotationAxis.Normalize();
			}
		}
	}

	if (ImGui::CollapsingHeader("Collision"))
	{
		ImGui::Checkbox("Is Platform", &body.isPlatform);
		ImGui::Checkbox("Push Entities", &body.pushEntities);
		ImGui::Checkbox("One Way Platform", &body.isOneWay);
		if (body.isOneWay)
		{
			ImGui::DragFloat3("One Way Direction", body.oneWayDirection.ptr(), 0.1f);
			if (body.oneWayDirection.LengthSqr() > 0.001f)
			{
				body.oneWayDirection.Normalize();
			}
		}
		ImGui::DragFloat("Voxel Size", &body.voxelSize, 0.1f, 0.1f, 10.0f);
	}

	if (ImGui::CollapsingHeader("Display"))
	{
		ImGui::Checkbox("Show Path", &platform.showPath);
	}
}

void PlatformEditor::RenderCreatePlatformDialog(World& world)
{
	ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Create Platform", &mShowCreateDialog))
	{
		char nameBuf[128] = "Platform";
		strncpy_s(nameBuf, mCreateInfo.name.c_str(), sizeof(nameBuf));
		if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
		{
			mCreateInfo.name = nameBuf;
		}

		ImGui::DragFloat3("Position", mCreateInfo.position.ptr(), 0.1f);
		ImGui::DragFloat3("Size", mCreateInfo.size.ptr(), 0.1f, 0.1f, 100.0f);

		const char* motionTypes[] = { "Static", "Linear", "Circular", "Custom" };
		int currentType = static_cast<int>(mCreateInfo.motionType);
		if (ImGui::Combo("Motion Type", &currentType, motionTypes, IM_ARRAYSIZE(motionTypes)))
		{
			mCreateInfo.motionType = static_cast<PlatformMotionType>(currentType);
		}

		ImGui::DragFloat("Voxel Size", &mCreateInfo.voxelSize, 0.1f, 0.1f, 10.0f);

		char modelPathBuf[260];
		std::string modelPath = WStringToUTF8(mCreateInfo.modelPath);
		strncpy_s(modelPathBuf, modelPath.c_str(), sizeof(modelPathBuf));
		if (ImGui::InputText("Model Path", modelPathBuf, sizeof(modelPathBuf)))
		{
			mCreateInfo.modelPath = UTF8ToWString(modelPathBuf);
		}

		ImGui::Separator();

		if (ImGui::Button("Create"))
		{
			if (mPlatformSystem)
			{
				mSelectedPlatform = mPlatformSystem->CreatePlatform(world, mCreateInfo);
			}
			mShowCreateDialog = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			mShowCreateDialog = false;
		}
	}
	ImGui::End();
}

void PlatformEditor::RenderGizmo(World& world, const Camera& camera)
{
	if (mSelectedPlatform == entt::null) return;

	auto& registry = world.GetRegistry();
	if (!registry.valid(mSelectedPlatform)) return;
	if (!registry.any_of<TransformComponent>(mSelectedPlatform)) return;

	auto& transform = registry.get<TransformComponent>(mSelectedPlatform);

	Matrix4x4 view = camera.GetViewMatrix();
	Matrix4x4 proj = camera.GetProjMatrix();

	Matrix4x4 worldMatrix = transform.GetMatrix();

	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

	ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
	switch (mGizmoOperation)
	{
	case GizmoOperation::Translate: op = ImGuizmo::TRANSLATE; break;
	case GizmoOperation::Rotate: op = ImGuizmo::ROTATE; break;
	case GizmoOperation::Scale: op = ImGuizmo::SCALE; break;
	}

	ImGuizmo::MODE mode = mUseLocalSpace ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

	float snap[3] = { mSnapTranslate, mSnapTranslate, mSnapTranslate };
	if (mGizmoOperation == GizmoOperation::Rotate)
	{
		snap[0] = snap[1] = snap[2] = mSnapRotate;
	}
	else if (mGizmoOperation == GizmoOperation::Scale)
	{
		snap[0] = snap[1] = snap[2] = mSnapScale;
	}

	float* snapPtr = mUseSnap ? snap : nullptr;

	if (ImGuizmo::Manipulate(*view.mat, *proj.mat, op, mode, *worldMatrix.mat, nullptr, snapPtr))
	{
		Vector3 newPos = worldMatrix.GetTrans();
		Quaternion newRot = worldMatrix.GetRotation();
		Vector3 newScale = worldMatrix.GetScale();

		transform.transition = newPos;
		transform.rotation = newRot;

		if (mGizmoOperation == GizmoOperation::Scale)
		{
			if (registry.any_of<PlatformComponent>(mSelectedPlatform))
			{
				auto& platform = registry.get<PlatformComponent>(mSelectedPlatform);
				platform.platformSize = newScale;
				transform.scale = newScale;

				if (registry.any_of<DynamicVoxelBodyComponent>(mSelectedPlatform))
				{
					auto& body = registry.get<DynamicVoxelBodyComponent>(mSelectedPlatform);
					body.halfExtentsInVoxels = newScale * 0.5f / body.voxelSize;
				}
			}
		}
	}
}

void PlatformEditor::RenderVisualization(World& world, const Matrix4x4& viewProj)
{
	auto& registry = world.GetRegistry();
	auto view = registry.view<TransformComponent, PlatformComponent, DynamicVoxelBodyComponent>();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& platform = view.get<PlatformComponent>(entity);
		auto& body = view.get<DynamicVoxelBodyComponent>(entity);

		Color boxColor = Color(0.3f, 0.6f, 0.9f, 1.0f);  // 默认蓝色
		if (platform.isSelected)
		{
			boxColor = Color::Yellow;
		}

		if (mShowCollisionBoxes)
		{
			DrawCollisionBox(transform, body, boxColor, viewProj);
		}

		if (mShowMotionPaths && platform.showPath)
		{
			DrawMotionPath(platform, viewProj);
		}
	}
}

void PlatformEditor::DrawMotionPath(const PlatformComponent& platform, const Matrix4x4& viewProj)
{
	Color pathColor = Color(1.0f, 1.0f, 0.0f, 1.0f);

	if (platform.motionType == PlatformMotionType::Linear)
	{
		Primitive::DrawLine(platform.startPosition, platform.endPosition, pathColor, viewProj);

		Vector3 markerSize(0.5f, 0.5f, 0.5f);
		Primitive::DrawCube(platform.startPosition, markerSize, Color::Green, viewProj);
		Primitive::DrawCube(platform.endPosition, markerSize, Color::Red, viewProj);
	}
	else if (platform.motionType == PlatformMotionType::Circular)
	{
		int segments = static_cast<int>(mPathVisualizationSegments);
		float angleStep = 6.28318530718f / segments;

		for (int i = 0; i < segments; ++i)
		{
			float angle1 = i * angleStep;
			float angle2 = (i + 1) * angleStep;

			Vector3 p1, p2;

			if (std::abs(platform.rotationAxis.y) > 0.9f)
			{
				p1 = platform.circleCenter + Vector3(
					platform.circleRadius * std::cos(angle1),
					0,
					platform.circleRadius * std::sin(angle1)
				);
				p2 = platform.circleCenter + Vector3(
					platform.circleRadius * std::cos(angle2),
					0,
					platform.circleRadius * std::sin(angle2)
				);
			}
			else if (std::abs(platform.rotationAxis.x) > 0.9f)
			{
				p1 = platform.circleCenter + Vector3(
					0,
					platform.circleRadius * std::cos(angle1),
					platform.circleRadius * std::sin(angle1)
				);
				p2 = platform.circleCenter + Vector3(
					0,
					platform.circleRadius * std::cos(angle2),
					platform.circleRadius * std::sin(angle2)
				);
			}
			else
			{
				p1 = platform.circleCenter + Vector3(
					platform.circleRadius * std::cos(angle1),
					platform.circleRadius * std::sin(angle1),
					0
				);
				p2 = platform.circleCenter + Vector3(
					platform.circleRadius * std::cos(angle2),
					platform.circleRadius * std::sin(angle2),
					0
				);
			}

			Primitive::DrawLine(p1, p2, pathColor, viewProj);
		}

		Vector3 markerSize(0.3f, 0.3f, 0.3f);
		Primitive::DrawCube(platform.circleCenter, markerSize, Color::Cyan, viewProj);
	}
}

void PlatformEditor::DrawCollisionBox(const TransformComponent& transform,
	const DynamicVoxelBodyComponent& body,
	const Color& color,
	const Matrix4x4& viewProj)
{
#ifndef RELEASE
	Vector3 halfExtents = body.halfExtentsInVoxels * body.voxelSize;
	Vector3 size = halfExtents * 2.0f;

	Primitive::DrawCube(transform.transition, size, color, viewProj);
#endif // !RELEASE
}
