#include "Matrix3x3.h"
#include "Matrix4x4.h"

namespace AtomEngine
{
	const Matrix3x3 Matrix3x3::ZERO(0, 0, 0, 0, 0, 0, 0, 0, 0);
	const Matrix3x3 Matrix3x3::IDENTITY(1, 0, 0, 0, 1, 0, 0, 0, 1);

	Matrix3x3::Matrix3x3(const Matrix4x4& m)
	{
		mat[0][0] = m.mat[0][0];
		mat[0][1] = m.mat[0][1];
		mat[0][2] = m.mat[0][2];

		mat[1][0] = m.mat[1][0];
		mat[1][1] = m.mat[1][1];
		mat[1][2] = m.mat[1][2];

		mat[2][0] = m.mat[2][0];
		mat[2][1] = m.mat[2][1];
		mat[2][2] = m.mat[2][2];
	}

	Matrix3x3::Matrix3x3(const Quaternion& q)
	{
		float xx = q.x * q.x;
		float yy = q.y * q.y;
		float zz = q.z * q.z;
		float xy = q.x * q.y;
		float xz = q.x * q.z;
		float yz = q.y * q.z;
		float wx = q.w * q.x;
		float wy = q.w * q.y;
		float wz = q.w * q.z;
		float ww = q.w * q.w;

		mat[0][0] = ww + xx - yy - zz;
		mat[0][1] = 2.0f * (xy + wz);
		mat[0][2] = 2.0f * (xz - wy);

		mat[1][0] = 2.0f * (xy - wz);
		mat[1][1] = ww - xx + yy - zz;
		mat[1][2] = 2.0f * (yz + wx);

		mat[2][0] = 2.0f * (xz + wy);
		mat[2][1] = 2.0f * (yz - wx);
		mat[2][2] = ww - xx - yy + zz;
	}

	Matrix3x3 Matrix3x3::MakeScale(const Vector3& v)
	{
		Matrix3x3 mat = Matrix3x3::IDENTITY;
		mat.mat[0][0] = v.x;
		mat.mat[1][1] = v.y;
		mat.mat[2][2] = v.z;
		return mat;
	}
}

