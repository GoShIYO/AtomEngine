#include "Math.h"
#include "Vector3.h"
#include "Matrix4x4.h"

namespace AtomEngine
{

	bool Math::Equal(float a, float b, float tolerance)
	{
		return std::fabs(b - a) <= tolerance;
	}

	float Math::DegreesToRadians(float degrees) { return degrees * deg2Rad; }

	float Math::RadiansToDegrees(float radians) { return radians * rad2Deg; }

	Radian Math::acos(float value)
	{
		if (-1.0 < value)
		{
			if (value < 1.0)
				return Radian(::acos(value));

			return Radian(0.0);
		}

		return Radian(PI);
	}

	Radian Math::asin(float value)
	{
		if (-1.0 < value)
		{
			if (value < 1.0)
				return Radian(::asin(value));

			return Radian(HalfPI);
		}

		return Radian(-HalfPI);
	}

	Matrix4x4 Math::MakeViewMatrix(const Vector3& position, const Quaternion& orientation, const Matrix4x4* reflectMatrix)
	{
		return Matrix4x4();
	}

	Matrix4x4 Math::MakeLookAtMatrix(const Vector3& eyePosition, const Vector3& targetPosition, const Vector3& upDir)
	{
		Vector3 f = (targetPosition - eyePosition).NormalizedCopy();
		Vector3 r = upDir.Cross(f).NormalizedCopy();
		Vector3 u = f.Cross(r);

		Matrix4x4 view = Matrix4x4::IDENTITY;

		view.mat[0][0] = r.x; view.mat[0][1] = r.y; view.mat[0][2] = r.z;
		view.mat[1][0] = u.x; view.mat[1][1] = u.y; view.mat[1][2] = u.z;
		view.mat[2][0] = f.x; view.mat[2][1] = f.y; view.mat[2][2] = f.z;

		view.mat[3][0] = -r.Dot(eyePosition);
		view.mat[3][1] = -u.Dot(eyePosition);
		view.mat[3][2] = -f.Dot(eyePosition);

		return view;
	}

	Matrix4x4 Math::MakePerspectiveMatrix(Radian fovy, float aspect, float znear, float zfar)
	{
		Matrix4x4 result = {
			1.0f / (aspect * tan(fovy * 0.5f)), 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f / tan(fovy * 0.5f), 0.0f, 0.0f,
			0.0f, 0.0f, zfar / (zfar - znear), 1.0f,
			0.0f, 0.0f, -zfar * znear / (zfar - znear), 0.0f
		};
		return result;
	}

	Matrix4x4 Math::MakeOrthographicProjectionMatrix(float left, float top, float right, float bottom, float nearClip, float farClip)
	{
		Matrix4x4 result = {
			2 / (right - left), 0.0f, 0.0f, 0.0f,
			0.0f, 2 / (top - bottom), 0.0f, 0.0f,
			0.0f, 0.0f, 1 / (farClip - nearClip), 0.0f,
			(left + right) / (left - right), (top + bottom) / (bottom - top), nearClip / (nearClip - farClip), 1.0f
		};
		return result;
	}
}