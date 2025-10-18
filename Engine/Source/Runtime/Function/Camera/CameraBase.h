#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include "Runtime/Core/Math/Frustum.h"

namespace AtomEngine
{
	class CameraBase
	{
	public:

		void Update();
		
		void SetLookAt(const Vector3& eye, const Vector3& lookAt, const Vector3& up);
		void SetLookDirection(const Vector3& forward, const Vector3& up);
		void SetRotation(const Quaternion& basisRotation);
		void SetRotation(const Vector3& rotation);
		void SetPosition(const Vector3& worldPos);
		void SetTransform(const Transform& transform);

		const Vector3 GetRightVec() const { return mBasis.GetX(); }
		const Vector3 GetUpVec() const { return mBasis.GetY(); }
		const Vector3 GetForwardVec() const { return mBasis.GetZ(); }

		const Quaternion GetRotation() const { return mCameraToWorld.rotation; }
		const Vector3 GetPosition() const { return mCameraToWorld.transition; }

		// さまざまな行列とフラスタを読み取るためのアクセサ
		const Matrix4x4& GetViewMatrix() const { return mViewMatrix; }
		const Matrix4x4& GetProjMatrix() const { return mProjMatrix; }
		const Matrix4x4& GetViewProjMatrix() const { return mViewProjMatrix; }
		const Matrix4x4& GetReprojectionMatrix() const { return mReprojectMatrix; }
		const Frustum& GetViewSpaceFrustum() const { return mFrustumVS; }
		const Frustum& GetWorldSpaceFrustum() const { return mFrustumWS; }

	protected:
		void SetProjMatrix(const Matrix4x4& ProjMat) { mProjMatrix = ProjMat; }

		Transform mCameraToWorld;

		Matrix4x4 mBasis;

		Matrix4x4 mViewMatrix;		// World -> View
		Matrix4x4 mProjMatrix;		// View -> Projection
		Matrix4x4 mViewProjMatrix;	// World -> Projection

		// 前のフレームの射影行列
		Matrix4x4 mPreviousViewProjMatrix;
		// クリップ空間座標を前のフレームの射影行列
		Matrix4x4 mReprojectMatrix;

		Frustum mFrustumVS;		// ビュー空間の視錐台
		Frustum mFrustumWS;		// ワールド空間の視錐台
	};

	class Camera : public CameraBase
	{
	public:
		Camera();

		void SetPerspectiveMatrix(float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip);
		void SetFOV(float verticalFovInRadians) { m_VerticalFOV = verticalFovInRadians; UpdateProjMatrix(); }
		void SetAspectRatio(float heightOverWidth) { m_AspectRatio = heightOverWidth; UpdateProjMatrix(); }
		void SetZRange(float nearZ, float farZ) { m_NearClip = nearZ; m_FarClip = farZ; UpdateProjMatrix(); }
		void ReverseZ(bool enable) { m_ReverseZ = enable; UpdateProjMatrix(); }

		float GetFOV() const { return m_VerticalFOV; }
		float GetNearClip() const { return m_NearClip; }
		float GetFarClip() const { return m_FarClip; }
		float GetClearDepth() const { return m_ReverseZ ? 1.0f : 0.0f; }

	private:

		void UpdateProjMatrix(void);

		float m_VerticalFOV;
		float m_AspectRatio;
		float m_NearClip;
		float m_FarClip;
		bool m_ReverseZ;
		bool m_InfiniteZ;
	};
}


