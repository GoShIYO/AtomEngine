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

		SetProjMatrix(Matrix4x4::MakeScale(Vector3(2.0f, 2.0f, 1.0f) * RcpDimensions));
		Update();

		mShadowMatrix = mViewProjMatrix * Matrix4x4(Matrix3x3::MakeScale({ 0.5f,-0.5f,1.0f }), Vector3(0.5f, 0.5f, 1.0f));
	}
}