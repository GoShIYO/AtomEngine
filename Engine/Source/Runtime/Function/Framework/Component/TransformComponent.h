#pragma once
#include "Runtime/Core/Math/Transform.h"

namespace AtomEngine
{
	class TransformComponent
	{
	public:
		TransformComponent(const Vector3& position, const Quaternion& rotation = Quaternion::IDENTITY, const Vector3& scale = { 1,1,1 })
			: mTransform(position, rotation, scale)
		{
		}
		TransformComponent() = default;
		~TransformComponent() = default;

		const Matrix4x4 GetMatrix() const { return mTransform.GetMatrix(); }

		void SetTranslation(const Vector3& v) { mTransform.transition = v; }
		void SetTranslation(float x, float y, float z) { mTransform.transition = Vector3(x, y, z); }
		void SetRotation(const Vector3& axis, Radian angle) { mTransform.rotation = Quaternion(axis, angle); }
		void SetRotation(const Quaternion& q) { mTransform.rotation = q; mTransform.rotation.Normalize(); }
		void SetScale(const Vector3& v) { mTransform.scale = v; }

		Vector3& GetTranslation() { return mTransform.transition; }
		Quaternion& GetRotation() { return mTransform.rotation; }
		Vector3& GetScale() { return mTransform.scale; }

		void SetParent(Transform* parent) { mTransform.parent = parent; }

	public:
		Transform mTransform;
	};
}
