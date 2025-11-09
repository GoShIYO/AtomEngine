#include "ShadowCamera.h"

namespace AtomEngine
{
	void ShadowCamera::UpdateMatrix(
		const Vector3& lightDir,
		const Vector3& shadowCenter,
		const Vector3& shadowBounds,
		uint32_t BufferWidth,
		uint32_t BufferHeight,
		uint32_t BufferPrecision
	)
	{
		SetLookDirection(lightDir, Vector3::FORWARD);

		Vector3 RcpDimensions = 1 / shadowBounds;
		Vector3 QuantizeScale = Vector3((float)BufferWidth, (float)BufferHeight, (float)((1 << BufferPrecision) - 1)) * RcpDimensions;

		Vector3 center = GetRotation().Conjugate() * shadowCenter;
		center = Math::Floor(center * QuantizeScale) / QuantizeScale;
		center = GetRotation() * center;

		SetPosition(center);

		float l = -shadowBounds.x * 0.5f;
		float r = shadowBounds.x * 0.5f;
		float b = -shadowBounds.y * 0.5f;
		float t = shadowBounds.y * 0.5f;
		float n = -shadowBounds.z * 0.5f;
		float f = shadowBounds.z * 0.5f;

		auto proj = Math::MakeOrthographicProjectionMatrix(l, r, b, t, n, f);

		SetProjMatrix(proj);

		Update();

		mShadowMatrix = mViewProjMatrix * Matrix4x4(Matrix3x3::MakeScale({ 0.5f,-0.5f,1.0f }), Vector3(0.5f, 0.5f, 0.0f));
	}
}