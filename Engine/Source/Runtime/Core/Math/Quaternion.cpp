#include "Quaternion.h"
#include "Vector3.h"
#include "Matrix3x3.h"
#include "Matrix4x4.h"

namespace AtomEngine
{
	const Quaternion Quaternion::ZERO(0, 0, 0, 0);
	const Quaternion Quaternion::IDENTITY(1, 0, 0, 0);

	const float Quaternion::kEpsilon = 1e-03;

	Quaternion Quaternion::operator*(const Quaternion& q) const
	{
		return Quaternion(
			x * q.w + w * q.x + y * q.z - z * q.y,
			y * q.w + w * q.y + z * q.x - x * q.z,
			z * q.w + w * q.z + x * q.y - y * q.x,
			w * q.w - x * q.x - y * q.y - z * q.z);
	}

	void Quaternion::FromRotationMatrix(const Matrix3x3& rotation)
	{
		float trace = rotation.mat[0][0] + rotation.mat[1][1] + rotation.mat[2][2];
		float root;

		if (trace > 0.0)
		{
			root = std::sqrt(trace + 1.0f);
			w = 0.5f * root;
			root = 0.5f / root;
			x = (rotation.mat[1][2] - rotation.mat[2][1]) * root;
			y = (rotation.mat[2][0] - rotation.mat[0][2]) * root;
			z = (rotation.mat[0][1] - rotation.mat[1][0]) * root;
		}
		else
		{
			size_t s_iNext[3] = { 1, 2, 0 };
			size_t i = 0;
			if (rotation.mat[1][1] > rotation.mat[0][0])
				i = 1;
			if (rotation.mat[2][2] > rotation.mat[i][i])
				i = 2;
			size_t j = s_iNext[i];
			size_t k = s_iNext[j];

			root = std::sqrt(rotation.mat[i][i] - rotation.mat[j][j] - rotation.mat[k][k] + 1.0f);
			float* apkQuat[3] = { &x, &y, &z };
			*apkQuat[i] = 0.5f * root;
			root = 0.5f / root;
			w = (rotation.mat[j][k] - rotation.mat[k][j]) * root;
			*apkQuat[j] = (rotation.mat[j][i] + rotation.mat[i][j]) * root;
			*apkQuat[k] = (rotation.mat[k][i] + rotation.mat[i][k]) * root;
		}
	}

	void Quaternion::ToRotationMatrix(Matrix3x3& kRot) const
	{
		float xx = x * x;
		float yy = y * y;
		float zz = z * z;
		float xy = x * y;
		float xz = x * z;
		float yz = y * z;
		float wx = w * x;
		float wy = w * y;
		float wz = w * z;

		kRot.mat[0][0] = 1.0f - 2.0f * (yy + zz);
		kRot.mat[0][1] = 2.0f * (xy + wz);
		kRot.mat[0][2] = 2.0f * (xz - wy);

		kRot.mat[1][0] = 2.0f * (xy + wz);
		kRot.mat[1][1] = 1.0f - 2.0f * (xx + zz);
		kRot.mat[1][2] = 2.0f * (yz - wx);

		kRot.mat[2][0] = 2.0f * (xz - wy);
		kRot.mat[2][1] = 2.0f * (yz + wx);
		kRot.mat[2][2] = 1.0f - 2.0f * (xx + yy);
	}

	void Quaternion::ToRotationMatrix(Matrix4x4& kRot) const
	{
		float xx = x * x;
		float yy = y * y;
		float zz = z * z;
		float xy = x * y;
		float xz = x * z;
		float yz = y * z;
		float wx = w * x;
		float wy = w * y;
		float wz = w * z;

		kRot.mat[0][0] = 1.0f - 2.0f * (yy + zz);
		kRot.mat[0][1] = 2.0f * (xy + wz);
		kRot.mat[0][2] = 2.0f * (xz - wy);
		kRot.mat[0][3] = 0.0f;

		kRot.mat[1][0] = 2.0f * (xy + wz);
		kRot.mat[1][1] = 1.0f - 2.0f * (xx + zz);
		kRot.mat[1][2] = 2.0f * (yz - wx);
		kRot.mat[1][3] = 0.0f;

		kRot.mat[2][0] = 2.0f * (xz - wy);
		kRot.mat[2][1] = 2.0f * (yz + wx);
		kRot.mat[2][2] = 1.0f - 2.0f * (xx + yy);
		kRot.mat[2][3] = 0.0f;

		kRot.mat[3][0] = 0.0f;
		kRot.mat[3][1] = 0.0f;
		kRot.mat[3][2] = 0.0f;
		kRot.mat[3][3] = 1.0f;
	}

	void Quaternion::FromAngleAxis(const Radian& angle, const Vector3& axis)
	{
		Radian halfAngle(0.5 * angle);
		float  sin_v = Math::sin(halfAngle);
		x = sin_v * axis.x;
		y = sin_v * axis.y;
		z = sin_v * axis.z;
		w = Math::cos(halfAngle);
	}

	Quaternion Quaternion::GetQuaternionFromAngleAxis(const Radian& angle, const Vector3& axis)
	{
		Quaternion q;
		q.FromAngleAxis(angle, axis);
		return q;
	}

	void Quaternion::FromDirection(const Vector3& direction, const Vector3& upDir)
	{
		Vector3 forward = direction;
		forward.z = 0.0f;
		forward.Normalize();

		Vector3 rightDir = upDir.Cross(forward);

		FromAxes(rightDir, forward, upDir);
		Normalize();
	}

	Quaternion Quaternion::GetQuaternionFromDirection(const Vector3& direction, const Vector3& upDir)
	{
		Quaternion orientation;
		orientation.FromDirection(direction, upDir);
		return orientation;
	}

	void Quaternion::ToAngleAxis(Radian& angle, Vector3& axis) const
	{
		float sqr_len = x * x + y * y + z * z;
		if (sqr_len > 0.0)
		{
			angle = 2.0 * Math::acos(w);
			float inv_len = Math::InvSqrt(sqr_len);
			axis.x = x * inv_len;
			axis.y = y * inv_len;
			axis.z = z * inv_len;
		}
		else
		{
			angle = Radian(0.0);
			axis.x = 1.0;
			axis.y = 0.0;
			axis.z = 0.0;
		}
	}

	void Quaternion::FromAxes(const Vector3& xaxis, const Vector3& yaxis, const Vector3& zaxis)
	{
		Matrix3x3 rot;

		rot.mat[0][0] = xaxis.x; 
		rot.mat[0][1] = xaxis.y; 
		rot.mat[0][2] = xaxis.z;

		rot.mat[1][0] = yaxis.x; 
		rot.mat[1][1] = yaxis.y; 
		rot.mat[1][2] = yaxis.z;

		rot.mat[2][0] = zaxis.x; 
		rot.mat[2][1] = zaxis.y; 
		rot.mat[2][2] = zaxis.z;

		FromRotationMatrix(rot);
	}

	Vector3 Quaternion::xAxis() const
	{
		float tx = 2.0 * x;
		float ty = 2.0f * y;
		float tz = 2.0f * z;
		float twy = ty * w;
		float twz = tz * w;
		float txy = ty * x;
		float txz = tz * x;
		float tyy = ty * y;
		float tzz = tz * z;

		return Vector3(1.0f - (tyy + tzz), txy + twz, txz - twy);
	}
	//-----------------------------------------------------------------------
	Vector3 Quaternion::yAxis() const
	{
		float tx = 2.0f * x;
		float ty = 2.0f * y;
		float tz = 2.0f * z;
		float twx = tx * w;
		float twz = tz * w;
		float txx = tx * x;
		float txy = ty * x;
		float tyz = tz * y;
		float tzz = tz * z;

		return Vector3(txy - twz, 1.0f - (txx + tzz), tyz + twx);
	}
	//-----------------------------------------------------------------------
	Vector3 Quaternion::zAxis() const
	{
		float tx = 2.0f * x;
		float ty = 2.0f * y;
		float tz = 2.0f * z;
		float twx = tx * w;
		float twy = ty * w;
		float txx = tx * x;
		float txz = tz * x;
		float tyy = ty * y;
		float tyz = tz * y;

		return Vector3(txz + twy, tyz - twx, 1.0f - (txx + tyy));
	}
	//-----------------------------------------------------------------------
	void Quaternion::ToAxes(Vector3& xaxis, Vector3& yaxis, Vector3& zaxis) const
	{
		Matrix3x3 rot;
		ToRotationMatrix(rot);

		xaxis.x = rot.mat[0][0]; 
		xaxis.y = rot.mat[0][1]; 
		xaxis.z = rot.mat[0][2];

		yaxis.x = rot.mat[1][0]; 
		yaxis.y = rot.mat[1][1]; 
		yaxis.z = rot.mat[1][2];

		zaxis.x = rot.mat[2][0]; 
		zaxis.y = rot.mat[2][1]; 
		zaxis.z = rot.mat[2][2];
	}

	Vector3 Quaternion::operator*(const Vector3& v) const
	{
		Vector3 uv, uuv;
		Vector3 qvec(x, y, z);
		uv = v.Cross(qvec);
		uuv = v.Cross(uv);
		uv *= (2.0f * w);
		uuv *= 2.0f;

		return v + uv + uuv;
	}

	Radian Quaternion::Yaw(bool reprojectAxis) const
	{
		if (reprojectAxis)
		{
			float ty = 2.0f * y;
			float tz = 2.0f * z;
			float txy = ty * x;
			float tyy = ty * y;
			float tzz = tz * z;
			float twz = tz * w;

			return Radian(Math::atan2(-(txy + twz), 1.0f - (tyy + tzz)));
		}
		else
		{
			return Radian(Math::atan2(-2 * (x * y + w * z), w * w + x * x - y * y - z * z));
		}
	}

	Radian Quaternion::Pitch(bool reprojectAxis) const
	{
		if (reprojectAxis)
		{
			float tx = 2.0f * x;
			float tz = 2.0f * z;
			float txx = tx * x;
			float tyz = tz * y;
			float tzz = tz * z;
			float twx = tx * w;

			return Radian(Math::atan2(tyz + twx, 1.0f - (txx + tzz)));
		}
		else
		{
			return Radian(Math::atan2(2 * (y * z + w * x), w * w - x * x - y * y + z * z));
		}
	}
	//-----------------------------------------------------------------------
	Radian Quaternion::Roll(bool reproject_axis) const
	{
		if (reproject_axis)
		{

			float tx = 2.0f * x;
			float ty = 2.0f * y;
			float tz = 2.0f * z;
			float twy = ty * w;
			float txx = tx * x;
			float txz = tz * x;
			float tyy = ty * y;

			return Radian(Math::atan2(txz + twy, 1.0f - (txx + tyy)));
		}
		else
		{
			return Radian(Math::asin(-2 * (x * z - w * y)));
		}
	}

	Quaternion Quaternion::SLerp(const Quaternion& kp, const Quaternion& kq, float t, bool shortestPath = false)
	{
		float      cos_v = kp.Dot(kq);
		Quaternion kt;

		if (cos_v < 0.0f && shortestPath)
		{
			cos_v = -cos_v;
			kt = -kq;
		}
		else
		{
			kt = kq;
		}

		if (Math::Abs(cos_v) < 1 - kEpsilon)
		{
			float  sin_v = Math::Sqrt(1 - Math::Sqr(cos_v));
			Radian angle = Math::atan2(sin_v, cos_v);
			float  inv_sin = 1.0f / sin_v;
			float  coeff0 = Math::sin((1.0f - t) * angle) * inv_sin;
			float  coeff1 = Math::sin(t * angle) * inv_sin;
			return coeff0 * kp + coeff1 * kt;
		}
		else
		{
			Quaternion r = (1.0f - t) * kp + t * kt;
			r.Normalize();
			return r;
		}
	}

	Quaternion Quaternion::Lerp(const Quaternion& kp, const Quaternion& kq, float t, bool shortestPath = false)
	{
		Quaternion result;
		float      cos_value = kp.Dot(kq);
		if (cos_value < 0.0f && shortestPath)
		{
			result = kp + t * ((-kq) - kp);
		}
		else
		{
			result = kp + t * (kq - kp);
		}
		result.Normalize();
		return result;
	}
}