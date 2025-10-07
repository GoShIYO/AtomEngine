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

		mat[0][0] = 1.0f - 2.0f * (yy + zz);
		mat[0][1] = 2.0f * (xy - wz);
		mat[0][2] = 2.0f * (xz + wy);

		mat[1][0] = 2.0f * (xy - wz);
		mat[1][1] = 1.0f - 2.0f * (xx + zz);
		mat[1][2] = 2.0f * (yz + wx);

		mat[2][0] = 2.0f * (xz + wy);
		mat[2][1] = 2.0f * (yz - wx);
		mat[2][2] = 1.0f - 2.0f * (xx + yy);
	}

	void AtomEngine::Matrix3x3::SetColumn(int col, const Vector3& v)
	{
		mat[0][col] = v.x;
		mat[1][col] = v.y;
        mat[2][col] = v.z;
	}	

	void AtomEngine::Matrix3x3::FromAxes(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis)
	{
		SetColumn(0, xAxis);
        SetColumn(1, yAxis);
        SetColumn(2, zAxis);
	}

    void Matrix3x3::CalculateQDUDecomposition(Matrix3x3& out_Q, Vector3& out_D, Vector3& out_U) const
    {
        Vector3 c0 = this->GetColumn(0);
        Vector3 c1 = this->GetColumn(1);
        Vector3 c2 = this->GetColumn(2);

        float len0 = c0.Length();
        if (!Math::Equal(len0, 0.0f))
            out_D.x = len0;
        else
            out_D.x = 0.0f;

        if (!Math::Equal(out_D.x, 0.0f))
            out_Q.SetColumn(0, c0 / out_D.x);
        else
            out_Q.SetColumn(0, Vector3::ZERO);

        float dot01 = out_Q.GetColumn(0).Dot(c1);
        out_U.x = dot01 / out_D.x;
        Vector3 c1_prime = c1 - out_U.x * c0;

        float len1 = c1_prime.Length();
        if (!Math::Equal(len1, 0.0f))
            out_D.y = len1;
        else
            out_D.y = 0.0f;

        if (!Math::Equal(out_D.y, 0.0f))
            out_Q.SetColumn(1, c1_prime / out_D.y);
        else
            out_Q.SetColumn(1, Vector3::ZERO);

        float dot02 = out_Q.GetColumn(0).Dot(c2);
        out_U.y = dot02 / out_D.x;

        float dot12 = out_Q.GetColumn(1).Dot(c2);
        out_U.z = dot12 / out_D.y;

        Vector3 c2_prime = c2 - out_U.y * c0 - out_U.z * c1_prime;

        float len2 = c2_prime.Length();
        if (!Math::Equal(len2, 0.0f))
            out_D.z = len2;
        else
            out_D.z = 0.0f;

        if (!Math::Equal(out_D.z, 0.0f))
            out_Q.SetColumn(2, c2_prime / out_D.z);
        else
            out_Q.SetColumn(2, Vector3::ZERO);

        if (out_Q.Determinant() < 0.0f)
        {
            out_Q.mat[0][2] = -out_Q.mat[0][2];
            out_Q.mat[1][2] = -out_Q.mat[1][2];
            out_Q.mat[2][2] = -out_Q.mat[2][2];
            out_D.z = -out_D.z;
            out_U.z = -out_U.z;
        }
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

