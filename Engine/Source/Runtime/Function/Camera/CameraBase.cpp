#include "CameraBase.h"


namespace AtomEngine
{
	void CameraBase::Update()
	{
		mPreviousViewProjMatrix = mViewProjMatrix;

		auto invRot = mCameraToWorld.rotation.Conjugate();

		auto view = Matrix4x4(invRot, invRot * -mCameraToWorld.transition);
		mViewMatrix = view;
		mViewProjMatrix = mViewMatrix * mProjMatrix;
		mReprojectMatrix = GetViewProjMatrix().Inverse() * mPreviousViewProjMatrix;

		mFrustumVS = Frustum(mProjMatrix);
		mFrustumWS = mCameraToWorld * mFrustumVS;
	}

	void CameraBase::SetLookAt(const Vector3& eye, const Vector3& lookAt, const Vector3& up)
	{
		SetLookDirection(lookAt - eye, up);
		SetPosition(eye);
	}

	void CameraBase::SetLookDirection(const Vector3& forward, const Vector3& up)
	{
		float forwardLenSq = forward.LengthSqr();
		auto f = Math::Select(forward * Math::Sqrt(1.0f / forwardLenSq), Vector3::FORWARD, forwardLenSq < Math::Epsilon);

		Vector3 right = Math::Cross(up, f);
		float rightLenSq = right.LengthSqr();
		right = Math::Select(right * Math::Sqrt(1.0f / rightLenSq),
			Quaternion(Radian(-Math::HalfPI), Vector3::UP) * forward,
			rightLenSq < Math::Epsilon);

		Vector3 u = Math::Cross(f, right);

		mBasis = Matrix4x4(right, u, f);
		mCameraToWorld.rotation = Quaternion(mBasis);
	}

	void CameraBase::SetRotation(const Quaternion& basisRotation)
	{
		mCameraToWorld.rotation = basisRotation.NormalizeCopy();
		mBasis = Matrix4x4(mCameraToWorld.rotation);
	}

	void CameraBase::SetRotation(const Vector3& rotation)
	{
		Quaternion qx = Quaternion(Radian(rotation.x), Vector3::RIGHT);
		Quaternion qy = Quaternion(Radian(rotation.y), Vector3::UP);
		Quaternion qz = Quaternion(Radian(rotation.z), Vector3::FORWARD);

		SetRotation(qz * qx * qy);
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

	Camera::Camera() : mReverseZ(true), mInfiniteZ(false)
	{
		SetPerspectiveMatrix(Math::PIDiv4, 9.0f / 16.0f, 1.0f, 1000.0f);
	}

	void Camera::SetPerspectiveMatrix(float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip)
	{
		mVerticalFOV = verticalFovRadians;
		mAspectRatio = aspectHeightOverWidth;
		mNearClip = nearZClip;
		mFarClip = farZClip;

		UpdateProjMatrix();

		mPreviousViewProjMatrix = mViewProjMatrix;
	}
	void Camera::UpdateProjMatrix(void)
	{
		float Y = 1.0f / std::tanf(mVerticalFOV * 0.5f);
		float X = Y * mAspectRatio;

		float Q1, Q2;

		if (mReverseZ)
		{
			if (mInfiniteZ)
			{
				Q1 = 0.0f;
				Q2 = mNearClip;
			}
			else
			{
				Q1 = mNearClip / (mNearClip - mFarClip);
				Q2 = -Q1 * mFarClip;
			}
		}
		else
		{
			if (mInfiniteZ)
			{
				Q1 = -1.0f;
				Q2 = -mNearClip;
			}
			else
			{
				Q1 = mFarClip / (mFarClip - mNearClip);
				Q2 = -Q1 * mNearClip;
			}
		}

		SetProjMatrix(Matrix4x4(
			Vector4(X, 0.0f, 0.0f, 0.0f),
			Vector4(0.0f, Y, 0.0f, 0.0f),
			Vector4(0.0f, 0.0f, Q1, 1.0f),
			Vector4(0.0f, 0.0f, Q2, 0.0f)
		));
	}
}


