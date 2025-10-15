#include "CameraBase.h"

namespace AtomEngine
{
	void CameraBase::Update()
	{
		mPreviousViewProjMatrix = mViewProjMatrix;

		auto invRot = mCameraToWorld.rotation.Conjugate();

		auto view = Matrix4x4(Matrix3x3(invRot), invRot * -mCameraToWorld.transition);
		mViewMatrix = view;
		mViewProjMatrix = Math::Multiply(mViewMatrix, mProjMatrix);
		mReprojectMatrix = mViewProjMatrix.Inverse() * mPreviousViewProjMatrix;

		mFrustumVS = Frustum(mProjMatrix);
		mFrustumWS = mCameraToWorld * mFrustumVS;
	}

	void CameraBase::SetEyeAtUp(const Vector3& eye, const Vector3& at, const Vector3& up)
	{
		SetLookDirection(at - eye, up);
		SetPosition(eye);
	}

	void CameraBase::SetLookDirection(const Vector3& forward, const Vector3& up)
	{
		float forwardLenSq = forward.LengthSqr();
		auto f = Math::Select(forward * std::sqrtf(1.0f / forwardLenSq), -Vector3::FORWARD, forwardLenSq < Math::Epsilon);

		Vector3 right = Math::Cross(f,up);

		float rightLenSq = right.LengthSqr();
		right = Math::Select(right * std::sqrtf(1.0f / rightLenSq),
			Quaternion(Radian(-Math::HalfPI), Vector3::UP) * forward,
			rightLenSq < Math::Epsilon);

		Vector3 u = Math::Cross(right,f);

		mBasis = Matrix4x4(right, u, -f);
		mCameraToWorld.rotation = Quaternion(mBasis);
	}

	void CameraBase::SetRotation(const Quaternion& basisRotation)
	{
		mCameraToWorld.rotation = basisRotation.NormalizeCopy();
		mBasis = Matrix4x4(mCameraToWorld.rotation);
	}

	void CameraBase::SetPosition(const Vector3& worldPos)
	{
		mCameraToWorld.transition = worldPos;
	}

	void CameraBase::SetTransform(const Transform& xform)
	{
		SetLookDirection(xform.GetMatrix().GetZ(), xform.GetMatrix().GetY());
		SetPosition(xform.transition);
	}

	Camera::Camera() : m_ReverseZ(true), m_InfiniteZ(false)
	{
		SetPerspectiveMatrix(Math::PIDiv4, 9.0f / 16.0f, 1.0f, 1000.0f);
	}

	void Camera::SetPerspectiveMatrix(float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip)
	{
		m_VerticalFOV = verticalFovRadians;
		m_AspectRatio = aspectHeightOverWidth;
		m_NearClip = nearZClip;
		m_FarClip = farZClip;

		UpdateProjMatrix();

		mPreviousViewProjMatrix = mViewProjMatrix;
	}
	void Camera::UpdateProjMatrix(void)
	{
		float Y = 1.0f / std::tanf(m_VerticalFOV * 0.5f);
		float X = Y * m_AspectRatio;

		float Q1, Q2;

		// ReverseZ puts far plane at Z=0 and near plane at Z=1.  This is never a bad idea, and it's
		// actually a great idea with F32 depth buffers to redistribute precision more evenly across
		// the entire range.  It requires clearing Z to 0.0f and using a GREATER variant depth test.
		// Some care must also be done to properly reconstruct linear W in a pixel shader from hyperbolic Z.
		if (m_ReverseZ)
		{
			if (m_InfiniteZ)
			{
				Q1 = 0.0f;
				Q2 = m_NearClip;
			}
			else
			{
				Q1 = m_NearClip / (m_FarClip - m_NearClip);
				Q2 = Q1 * m_FarClip;
			}
		}
		else
		{
			if (m_InfiniteZ)
			{
				Q1 = -1.0f;
				Q2 = -m_NearClip;
			}
			else
			{
				Q1 = m_FarClip / (m_NearClip - m_FarClip);
				Q2 = Q1 * m_NearClip;
			}
		}

		SetProjMatrix(Matrix4x4(
			Vector4(X, 0.0f, 0.0f, 0.0f),
			Vector4(0.0f, Y, 0.0f, 0.0f),
			Vector4(0.0f, 0.0f, Q1, -1.0f),
			Vector4(0.0f, 0.0f, Q2, 0.0f)
		));
	}
}
