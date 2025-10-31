#pragma once
#include "Runtime/Core/Math/Transform.h"

namespace AtomEngine
{
	class TransformComponent
	{
	public:
		~TransformComponent() = default;

		const Matrix4x4& GetWorldMatrix() const{return mTransform.GetMatrix();}

		void Translate(const Vector3& v) { mTransform.transition = v; }
		void Translate(float x, float y, float z) { mTransform.transition = Vector3(x, y, z); }
        void Rotate(const Vector3& axis,float angle) { mTransform.rotation = Quaternion(axis,angle); }
        void Rotate(const Quaternion& q) { mTransform.rotation = q; }
        void Scale(const Vector3& v) { mTransform.scale = v; }
		
	private:
		Transform mTransform;
	};
}
