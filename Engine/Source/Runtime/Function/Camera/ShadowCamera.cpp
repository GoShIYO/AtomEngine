#include "ShadowCamera.h"

namespace AtomEngine
{
	void ShadowCamera::UpdateMatrix(
		const Vector3& lightDir,
		const Vector3& shadowCenter,
		const Vector3& shadowBounds,
		uint32_t BufferWidth,
		uint32_t BufferHeight
	)
	{
		float radius = shadowBounds.Length();

		Vector3 lightPos = -2.0f * radius * lightDir + shadowCenter;
		mViewMatrix = Math::MakeLookAtMatrix(lightPos, shadowCenter, Vector3::UP);

		Vector3 sphereCenterLS = shadowCenter * mViewMatrix;

		float l = sphereCenterLS.x - radius;
		float r = sphereCenterLS.x + radius;
		float b = sphereCenterLS.y - radius;
		float t = sphereCenterLS.y + radius;
		float n = sphereCenterLS.z - radius;
		float f = sphereCenterLS.z + radius;

		SetProjMatrix(Math::MakeOrthographicProjectionMatrix(l, r, b, t, n, f));
		mViewProjMatrix = mViewMatrix * mProjMatrix;

		Matrix4x4 ndcToUV = Matrix4x4(
			Matrix3x3::MakeScale({ 0.5f, -0.5f, 1.0f }),
			Vector3(0.5f, 0.5f, 0.0f)
		);

		mShadowMatrix = mViewProjMatrix * ndcToUV;
	}
}