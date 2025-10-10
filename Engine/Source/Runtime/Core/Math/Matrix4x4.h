#pragma once
#include "Math.h"
#include "Quaternion.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix3x3.h"

namespace AtomEngine
{
	class Matrix4x4
	{
	public:
		float mat[4][4];

	public:
		Matrix4x4(const Matrix4x4& m)
		{
			mat[0][0] = m.mat[0][0];
			mat[0][1] = m.mat[0][1];
			mat[0][2] = m.mat[0][2];
			mat[0][3] = m.mat[0][3];
			mat[1][0] = m.mat[1][0];
			mat[1][1] = m.mat[1][1];
			mat[1][2] = m.mat[1][2];
			mat[1][3] = m.mat[1][3];
			mat[2][0] = m.mat[2][0];
			mat[2][1] = m.mat[2][1];
			mat[2][2] = m.mat[2][2];
			mat[2][3] = m.mat[2][3];
			mat[3][0] = m.mat[3][0];
			mat[3][1] = m.mat[3][1];
			mat[3][2] = m.mat[3][2];
			mat[3][3] = m.mat[3][3];
		}

		Matrix4x4() { operator=(IDENTITY); }

		Matrix4x4(const float(&float_array)[16])
		{
			mat[0][0] = float_array[0];
			mat[0][1] = float_array[1];
			mat[0][2] = float_array[2];
			mat[0][3] = float_array[3];
			mat[1][0] = float_array[4];
			mat[1][1] = float_array[5];
			mat[1][2] = float_array[6];
			mat[1][3] = float_array[7];
			mat[2][0] = float_array[8];
			mat[2][1] = float_array[9];
			mat[2][2] = float_array[10];
			mat[2][3] = float_array[11];
			mat[3][0] = float_array[12];
			mat[3][1] = float_array[13];
			mat[3][2] = float_array[14];
			mat[3][3] = float_array[15];
		}

		Matrix4x4(float m00,
			float m01,
			float m02,
			float m03,
			float m10,
			float m11,
			float m12,
			float m13,
			float m20,
			float m21,
			float m22,
			float m23,
			float m30,
			float m31,
			float m32,
			float m33)
		{
			mat[0][0] = m00;
			mat[0][1] = m01;
			mat[0][2] = m02;
			mat[0][3] = m03;
			mat[1][0] = m10;
			mat[1][1] = m11;
			mat[1][2] = m12;
			mat[1][3] = m13;
			mat[2][0] = m20;
			mat[2][1] = m21;
			mat[2][2] = m22;
			mat[2][3] = m23;
			mat[3][0] = m30;
			mat[3][1] = m31;
			mat[3][2] = m32;
			mat[3][3] = m33;
		}

		Matrix4x4(const Vector4& row0, const Vector4& row1, const Vector4& row2, const Vector4& row3)
		{
			mat[0][0] = row0.x;
			mat[0][1] = row0.y;
			mat[0][2] = row0.z;
			mat[0][3] = row0.w;
			mat[1][0] = row1.x;
			mat[1][1] = row1.y;
			mat[1][2] = row1.z;
			mat[1][3] = row1.w;
			mat[2][0] = row2.x;
			mat[2][1] = row2.y;
			mat[2][2] = row2.z;
			mat[2][3] = row2.w;
			mat[3][0] = row3.x;
			mat[3][1] = row3.y;
			mat[3][2] = row3.z;
			mat[3][3] = row3.w;
		}

		Matrix4x4(float* float_array)
		{
            mat[0][0] = float_array[0];
			mat[0][1] = float_array[1];
			mat[0][2] = float_array[2];
			mat[0][3] = float_array[3];
			mat[1][0] = float_array[4];
			mat[1][1] = float_array[5];
			mat[1][2] = float_array[6];
			mat[1][3] = float_array[7];
			mat[2][0] = float_array[8];
			mat[2][1] = float_array[9];
			mat[2][2] = float_array[10];
			mat[2][3] = float_array[11];
			mat[3][0] = float_array[12];
			mat[3][1] = float_array[13];
			mat[3][2] = float_array[14];
			mat[3][3] = float_array[15];
		}

		Matrix4x4(const Vector3& scale, const Quaternion& rotation, const Vector3& position)
		{
			MakeAffine(scale, rotation, position);
		}

		Matrix4x4(const Vector3& x, const Vector3& y, const Vector3& z)
		{
            SetMatrix3x3(Matrix3x3(x, y, z));
		}

		Matrix4x4(const Matrix3x3& xyz,const Vector3& w)
		{
			mat[0][0] = xyz.mat[0][0];
			mat[0][1] = xyz.mat[0][1];
			mat[0][2] = xyz.mat[0][2];
			mat[0][3] = 0.0f;

			mat[1][0] = xyz.mat[1][0];
			mat[1][1] = xyz.mat[1][1];
			mat[1][2] = xyz.mat[1][2];
			mat[1][3] = 0.0f;

			mat[2][0] = xyz.mat[2][0];
			mat[2][1] = xyz.mat[2][1];
			mat[2][2] = xyz.mat[2][2];
			mat[2][3] = 0.0f;

			mat[3][0] = w.x;
			mat[3][1] = w.y;
			mat[3][2] = w.z;
			mat[3][3] = 1.0f;
		}

		Matrix4x4(const Quaternion& rot, const Vector3& position)
		{
			*this = Matrix4x4(Matrix3x3(rot), position);
		}

		void FromData(const float(&float_array)[16])
		{
			mat[0][0] = float_array[0];
			mat[0][1] = float_array[1];
			mat[0][2] = float_array[2];
			mat[0][3] = float_array[3];
			mat[1][0] = float_array[4];
			mat[1][1] = float_array[5];
			mat[1][2] = float_array[6];
			mat[1][3] = float_array[7];
			mat[2][0] = float_array[8];
			mat[2][1] = float_array[9];
			mat[2][2] = float_array[10];
			mat[2][3] = float_array[11];
			mat[3][0] = float_array[12];
			mat[3][1] = float_array[13];
			mat[3][2] = float_array[14];
			mat[3][3] = float_array[15];
		}

		void ToData(float(&float_array)[16]) const
		{
			float_array[0] = mat[0][0];
			float_array[1] = mat[0][1];
			float_array[2] = mat[0][2];
			float_array[3] = mat[0][3];
			float_array[4] = mat[1][0];
			float_array[5] = mat[1][1];
			float_array[6] = mat[1][2];
			float_array[7] = mat[1][3];
			float_array[8] = mat[2][0];
			float_array[9] = mat[2][1];
			float_array[10] = mat[2][2];
			float_array[11] = mat[2][3];
			float_array[12] = mat[3][0];
			float_array[13] = mat[3][1];
			float_array[14] = mat[3][2];
			float_array[15] = mat[3][3];
		}

		void SetMatrix3x3(const Matrix3x3& mat3)
		{
			mat[0][0] = mat3.mat[0][0];
			mat[0][1] = mat3.mat[0][1];
			mat[0][2] = mat3.mat[0][2];
			mat[0][3] = 0;

			mat[1][0] = mat3.mat[1][0];
			mat[1][1] = mat3.mat[1][1];
			mat[1][2] = mat3.mat[1][2];
			mat[1][3] = 0;

			mat[2][0] = mat3.mat[2][0];
			mat[2][1] = mat3.mat[2][1];
			mat[2][2] = mat3.mat[2][2];
			mat[2][3] = 0;

			mat[3][0] = 0;
			mat[3][1] = 0;
			mat[3][2] = 0;
			mat[3][3] = 1;
		}

		Matrix4x4(const Quaternion& rot)
		{
			Matrix3x3 m3x3;
			rot.ToRotationMatrix(m3x3);
			operator=(IDENTITY);
			SetMatrix3x3(m3x3);
		}

		float* operator[](size_t row_index)
		{
			assert(row_index < 4);
			return mat[row_index];
		}

		const float* operator[](size_t row_index) const
		{
			assert(row_index < 4);
			return mat[row_index];
		}

		Matrix4x4 operator*(const Matrix4x4& m2) const
		{
			Matrix4x4 r;
			r.mat[0][0] = mat[0][0] * m2.mat[0][0] + mat[0][1] * m2.mat[1][0] + mat[0][2] * m2.mat[2][0] +
				mat[0][3] * m2.mat[3][0];
			r.mat[0][1] = mat[0][0] * m2.mat[0][1] + mat[0][1] * m2.mat[1][1] + mat[0][2] * m2.mat[2][1] +
				mat[0][3] * m2.mat[3][1];
			r.mat[0][2] = mat[0][0] * m2.mat[0][2] + mat[0][1] * m2.mat[1][2] + mat[0][2] * m2.mat[2][2] +
				mat[0][3] * m2.mat[3][2];
			r.mat[0][3] = mat[0][0] * m2.mat[0][3] + mat[0][1] * m2.mat[1][3] + mat[0][2] * m2.mat[2][3] +
				mat[0][3] * m2.mat[3][3];

			r.mat[1][0] = mat[1][0] * m2.mat[0][0] + mat[1][1] * m2.mat[1][0] + mat[1][2] * m2.mat[2][0] +
				mat[1][3] * m2.mat[3][0];
			r.mat[1][1] = mat[1][0] * m2.mat[0][1] + mat[1][1] * m2.mat[1][1] + mat[1][2] * m2.mat[2][1] +
				mat[1][3] * m2.mat[3][1];
			r.mat[1][2] = mat[1][0] * m2.mat[0][2] + mat[1][1] * m2.mat[1][2] + mat[1][2] * m2.mat[2][2] +
				mat[1][3] * m2.mat[3][2];
			r.mat[1][3] = mat[1][0] * m2.mat[0][3] + mat[1][1] * m2.mat[1][3] + mat[1][2] * m2.mat[2][3] +
				mat[1][3] * m2.mat[3][3];

			r.mat[2][0] = mat[2][0] * m2.mat[0][0] + mat[2][1] * m2.mat[1][0] + mat[2][2] * m2.mat[2][0] +
				mat[2][3] * m2.mat[3][0];
			r.mat[2][1] = mat[2][0] * m2.mat[0][1] + mat[2][1] * m2.mat[1][1] + mat[2][2] * m2.mat[2][1] +
				mat[2][3] * m2.mat[3][1];
			r.mat[2][2] = mat[2][0] * m2.mat[0][2] + mat[2][1] * m2.mat[1][2] + mat[2][2] * m2.mat[2][2] +
				mat[2][3] * m2.mat[3][2];
			r.mat[2][3] = mat[2][0] * m2.mat[0][3] + mat[2][1] * m2.mat[1][3] + mat[2][2] * m2.mat[2][3] +
				mat[2][3] * m2.mat[3][3];

			r.mat[3][0] = mat[3][0] * m2.mat[0][0] + mat[3][1] * m2.mat[1][0] + mat[3][2] * m2.mat[2][0] +
				mat[3][3] * m2.mat[3][0];
			r.mat[3][1] = mat[3][0] * m2.mat[0][1] + mat[3][1] * m2.mat[1][1] + mat[3][2] * m2.mat[2][1] +
				mat[3][3] * m2.mat[3][1];
			r.mat[3][2] = mat[3][0] * m2.mat[0][2] + mat[3][1] * m2.mat[1][2] + mat[3][2] * m2.mat[2][2] +
				mat[3][3] * m2.mat[3][2];
			r.mat[3][3] = mat[3][0] * m2.mat[0][3] + mat[3][1] * m2.mat[1][3] + mat[3][2] * m2.mat[2][3] +
				mat[3][3] * m2.mat[3][3];

			return r;
		}

		Vector3 operator*(const Vector3& v) const
		{
			Vector3 r;

			float inv_w = 1.0f / (mat[3][0] * v.x + mat[3][1] * v.y + mat[3][2] * v.z + mat[3][3]);

			r.x = (mat[0][0] * v.x + mat[0][1] * v.y + mat[0][2] * v.z + mat[0][3]) * inv_w;
			r.y = (mat[1][0] * v.x + mat[1][1] * v.y + mat[1][2] * v.z + mat[1][3]) * inv_w;
			r.z = (mat[2][0] * v.x + mat[2][1] * v.y + mat[2][2] * v.z + mat[2][3]) * inv_w;

			return r;
		}

		Vector4 operator*(const Vector4& v) const
		{
			return Vector4(mat[0][0] * v.x + mat[0][1] * v.y + mat[0][2] * v.z + mat[0][3] * v.w,
				mat[1][0] * v.x + mat[1][1] * v.y + mat[1][2] * v.z + mat[1][3] * v.w,
				mat[2][0] * v.x + mat[2][1] * v.y + mat[2][2] * v.z + mat[2][3] * v.w,
				mat[3][0] * v.x + mat[3][1] * v.y + mat[3][2] * v.z + mat[3][3] * v.w);
		}


		Matrix4x4 operator+(const Matrix4x4& m2) const
		{
			Matrix4x4 r;

			r.mat[0][0] = mat[0][0] + m2.mat[0][0];
			r.mat[0][1] = mat[0][1] + m2.mat[0][1];
			r.mat[0][2] = mat[0][2] + m2.mat[0][2];
			r.mat[0][3] = mat[0][3] + m2.mat[0][3];

			r.mat[1][0] = mat[1][0] + m2.mat[1][0];
			r.mat[1][1] = mat[1][1] + m2.mat[1][1];
			r.mat[1][2] = mat[1][2] + m2.mat[1][2];
			r.mat[1][3] = mat[1][3] + m2.mat[1][3];

			r.mat[2][0] = mat[2][0] + m2.mat[2][0];
			r.mat[2][1] = mat[2][1] + m2.mat[2][1];
			r.mat[2][2] = mat[2][2] + m2.mat[2][2];
			r.mat[2][3] = mat[2][3] + m2.mat[2][3];

			r.mat[3][0] = mat[3][0] + m2.mat[3][0];
			r.mat[3][1] = mat[3][1] + m2.mat[3][1];
			r.mat[3][2] = mat[3][2] + m2.mat[3][2];
			r.mat[3][3] = mat[3][3] + m2.mat[3][3];

			return r;
		}


		Matrix4x4 operator-(const Matrix4x4& m2) const
		{
			Matrix4x4 r;
			r.mat[0][0] = mat[0][0] - m2.mat[0][0];
			r.mat[0][1] = mat[0][1] - m2.mat[0][1];
			r.mat[0][2] = mat[0][2] - m2.mat[0][2];
			r.mat[0][3] = mat[0][3] - m2.mat[0][3];

			r.mat[1][0] = mat[1][0] - m2.mat[1][0];
			r.mat[1][1] = mat[1][1] - m2.mat[1][1];
			r.mat[1][2] = mat[1][2] - m2.mat[1][2];
			r.mat[1][3] = mat[1][3] - m2.mat[1][3];

			r.mat[2][0] = mat[2][0] - m2.mat[2][0];
			r.mat[2][1] = mat[2][1] - m2.mat[2][1];
			r.mat[2][2] = mat[2][2] - m2.mat[2][2];
			r.mat[2][3] = mat[2][3] - m2.mat[2][3];

			r.mat[3][0] = mat[3][0] - m2.mat[3][0];
			r.mat[3][1] = mat[3][1] - m2.mat[3][1];
			r.mat[3][2] = mat[3][2] - m2.mat[3][2];
			r.mat[3][3] = mat[3][3] - m2.mat[3][3];

			return r;
		}

		Matrix4x4 operator*(float scalar) const
		{
			return Matrix4x4(scalar * mat[0][0],
				scalar * mat[0][1],
				scalar * mat[0][2],
				scalar * mat[0][3],
				scalar * mat[1][0],
				scalar * mat[1][1],
				scalar * mat[1][2],
				scalar * mat[1][3],
				scalar * mat[2][0],
				scalar * mat[2][1],
				scalar * mat[2][2],
				scalar * mat[2][3],
				scalar * mat[3][0],
				scalar * mat[3][1],
				scalar * mat[3][2],
				scalar * mat[3][3]);
		}


		bool operator==(const Matrix4x4& m2) const
		{
			return !(mat[0][0] != m2.mat[0][0] || mat[0][1] != m2.mat[0][1] || mat[0][2] != m2.mat[0][2] ||
				mat[0][3] != m2.mat[0][3] || mat[1][0] != m2.mat[1][0] || mat[1][1] != m2.mat[1][1] ||
				mat[1][2] != m2.mat[1][2] || mat[1][3] != m2.mat[1][3] || mat[2][0] != m2.mat[2][0] ||
				mat[2][1] != m2.mat[2][1] || mat[2][2] != m2.mat[2][2] || mat[2][3] != m2.mat[2][3] ||
				mat[3][0] != m2.mat[3][0] || mat[3][1] != m2.mat[3][1] || mat[3][2] != m2.mat[3][2] ||
				mat[3][3] != m2.mat[3][3]);
		}

		bool operator!=(const Matrix4x4& m2) const
		{
			return mat[0][0] != m2.mat[0][0] || mat[0][1] != m2.mat[0][1] || mat[0][2] != m2.mat[0][2] ||
				mat[0][3] != m2.mat[0][3] || mat[1][0] != m2.mat[1][0] || mat[1][1] != m2.mat[1][1] ||
				mat[1][2] != m2.mat[1][2] || mat[1][3] != m2.mat[1][3] || mat[2][0] != m2.mat[2][0] ||
				mat[2][1] != m2.mat[2][1] || mat[2][2] != m2.mat[2][2] || mat[2][3] != m2.mat[2][3] ||
				mat[3][0] != m2.mat[3][0] || mat[3][1] != m2.mat[3][1] || mat[3][2] != m2.mat[3][2] ||
				mat[3][3] != m2.mat[3][3];
		}

		Matrix4x4 Transpose() const
		{
			return Matrix4x4(mat[0][0],
				mat[1][0],
				mat[2][0],
				mat[3][0],
				mat[0][1],
				mat[1][1],
				mat[2][1],
				mat[3][1],
				mat[0][2],
				mat[1][2],
				mat[2][2],
				mat[3][2],
				mat[0][3],
				mat[1][3],
				mat[2][3],
				mat[3][3]);
		}

		//-----------------------------------------------------------------------
		float GetMinor(size_t r0, size_t r1, size_t r2, size_t c0, size_t c1, size_t c2) const
		{
			return mat[r0][c0] * (mat[r1][c1] * mat[r2][c2] - mat[r2][c1] * mat[r1][c2]) -
				mat[r0][c1] * (mat[r1][c0] * mat[r2][c2] - mat[r2][c0] * mat[r1][c2]) +
				mat[r0][c2] * (mat[r1][c0] * mat[r2][c1] - mat[r2][c0] * mat[r1][c1]);
		}

		void setTrans(const Vector3& v)
		{
			mat[3][0] = v.x;
			mat[3][1] = v.y;
			mat[3][2] = v.z;
		}

		Vector3 GetTrans() const { return Vector3(mat[0][3], mat[1][3], mat[2][3]); }

		Quaternion GetRotation() const
		{
			Matrix3x3 mat3(*this);
			return mat3.GetRotation();
		}

		Matrix4x4 MakeViewportMatrix(uint32_t width, uint32_t height)
		{

			Matrix4x4 result = Matrix4x4::IDENTITY;

			result.mat[0][0] = (float)width * 0.5f;
			result.mat[3][0] = (float)width * 0.5f;
			result.mat[1][1] = (float)height * -0.5f;
			result.mat[3][1] = (float)height * 0.5f;
			result.mat[2][2] = 1.0f;
			result.mat[3][2] = 0.0f;

			return result;
		}

		static Matrix4x4 MirrorMatrix(const Vector4& mirrorPlane)
		{
			Vector3 n(mirrorPlane.x, mirrorPlane.y, mirrorPlane.z);
			float d = mirrorPlane.w;

			float len2 = n.LengthSqr();
			if (len2 <= 1e-8f)
			{
				return Matrix4x4::IDENTITY;
			}
			float invLen = 1.0f / std::sqrt(len2);
			n.x *= invLen; n.y *= invLen; n.z *= invLen;
			d *= invLen;

			Matrix4x4 result;

			result.mat[0][0] = 1.0f - 2.0f * n.x * n.x;
			result.mat[0][1] = -2.0f * n.x * n.y;
			result.mat[0][2] = -2.0f * n.x * n.z;
			result.mat[0][3] = -2.0f * d * n.x;

			result.mat[1][0] = -2.0f * n.y * n.x;
			result.mat[1][1] = 1.0f - 2.0f * n.y * n.y;
			result.mat[1][2] = -2.0f * n.y * n.z;
			result.mat[1][3] = -2.0f * d * n.y;

			result.mat[2][0] = -2.0f * n.z * n.x;
			result.mat[2][1] = -2.0f * n.z * n.y;
			result.mat[2][2] = 1.0f - 2.0f * n.z * n.z;
			result.mat[2][3] = -2.0f * d * n.z;

			result.mat[3][0] = 0.0f;
			result.mat[3][1] = 0.0f;
			result.mat[3][2] = 0.0f;
			result.mat[3][3] = 1.0f;

			return result;
		}


		static Matrix4x4 RotationMatrix(Vector3 normal)
		{
			Vector3 up = Vector3(0, 0, 1);
			if (fabs(normal.z) > 0.999f)
			{
				up = Vector3(0, 1, 0);
			}

			Vector3 right = up.Cross(normal);
			up = normal.Cross(right);

			right.Normalize();
			up.Normalize();

			Matrix4x4 result = Matrix4x4::IDENTITY;
			result.SetMatrix3x3(Matrix3x3(right, up, normal));

			return result;
		}

		void MakeTrans(const Vector3& v)
		{
			mat[0][0] = 1.0f;
			mat[0][1] = 0.0f;
			mat[0][2] = 0.0f;
			mat[0][3] = 0.0f;

			mat[1][0] = 0.0f;
			mat[1][1] = 1.0f;
			mat[1][2] = 0.0f;
			mat[1][3] = 0.0f;

			mat[2][0] = 0.0f;
			mat[2][1] = 0.0f;
			mat[2][2] = 1.0f;
			mat[2][3] = 0.0f;

			mat[3][0] = v.x;
			mat[3][1] = v.y;
			mat[3][2] = v.y;
			mat[3][3] = 1.0;
		}

		void MakeTrans(float tx, float ty, float tz)
		{
			mat[0][0] = 1.0;
			mat[0][1] = 0.0;
			mat[0][2] = 0.0;
			mat[0][3] = tx;
			mat[1][0] = 0.0;
			mat[1][1] = 1.0;
			mat[1][2] = 0.0;
			mat[1][3] = ty;
			mat[2][0] = 0.0;
			mat[2][1] = 0.0;
			mat[2][2] = 1.0;
			mat[2][3] = tz;
			mat[3][0] = 0.0;
			mat[3][1] = 0.0;
			mat[3][2] = 0.0;
			mat[3][3] = 1.0;
		}

		static Matrix4x4 MakeTransMatrix(const Vector3& v)
		{
			Matrix4x4 r;

			r.mat[0][0] = 1.0;
			r.mat[0][1] = 0.0;
			r.mat[0][2] = 0.0;
			r.mat[0][3] = v.x;
			r.mat[1][0] = 0.0;
			r.mat[1][1] = 1.0;
			r.mat[1][2] = 0.0;
			r.mat[1][3] = v.y;
			r.mat[2][0] = 0.0;
			r.mat[2][1] = 0.0;
			r.mat[2][2] = 1.0;
			r.mat[2][3] = v.z;
			r.mat[3][0] = 0.0;
			r.mat[3][1] = 0.0;
			r.mat[3][2] = 0.0;
			r.mat[3][3] = 1.0;

			return r;
		}

		static Matrix4x4 GetTrans(float t_x, float t_y, float t_z)
		{
			Matrix4x4 r;

			r.mat[0][0] = 1.0;
			r.mat[0][1] = 0.0;
			r.mat[0][2] = 0.0;
			r.mat[0][3] = t_x;
			r.mat[1][0] = 0.0;
			r.mat[1][1] = 1.0;
			r.mat[1][2] = 0.0;
			r.mat[1][3] = t_y;
			r.mat[2][0] = 0.0;
			r.mat[2][1] = 0.0;
			r.mat[2][2] = 1.0;
			r.mat[2][3] = t_z;
			r.mat[3][0] = 0.0;
			r.mat[3][1] = 0.0;
			r.mat[3][2] = 0.0;
			r.mat[3][3] = 1.0;

			return r;
		}

		void SetScale(const Vector3& v)
		{
			mat[0][0] = v.x;
			mat[1][1] = v.y;
			mat[2][2] = v.z;
		}

		static Matrix4x4 MakeScaleMatrix(const Vector3& v)
		{
			Matrix4x4 r;
			r.mat[0][0] = v.x;
			r.mat[0][1] = 0.0;
			r.mat[0][2] = 0.0;
			r.mat[0][3] = 0.0;
			r.mat[1][0] = 0.0;
			r.mat[1][1] = v.y;
			r.mat[1][2] = 0.0;
			r.mat[1][3] = 0.0;
			r.mat[2][0] = 0.0;
			r.mat[2][1] = 0.0;
			r.mat[2][2] = v.z;
			r.mat[2][3] = 0.0;
			r.mat[3][0] = 0.0;
			r.mat[3][1] = 0.0;
			r.mat[3][2] = 0.0;
			r.mat[3][3] = 1.0;

			return r;
		}

		static Matrix4x4 MakeScaleMatrix(float s_x, float s_y, float s_z)
		{
			Matrix4x4 r;
			r.mat[0][0] = s_x;
			r.mat[0][1] = 0.0;
			r.mat[0][2] = 0.0;
			r.mat[0][3] = 0.0;
			r.mat[1][0] = 0.0;
			r.mat[1][1] = s_y;
			r.mat[1][2] = 0.0;
			r.mat[1][3] = 0.0;
			r.mat[2][0] = 0.0;
			r.mat[2][1] = 0.0;
			r.mat[2][2] = s_z;
			r.mat[2][3] = 0.0;
			r.mat[3][0] = 0.0;
			r.mat[3][1] = 0.0;
			r.mat[3][2] = 0.0;
			r.mat[3][3] = 1.0;

			return r;
		}

		static Matrix4x4 MakeRotateXMatrix(Radian radian)
		{
			float cos = Math::cos(radian);
			float sin = Math::sin(radian);

			return Matrix4x4(
				1.0, 0.0, 0.0, 0.0,
				0.0, cos, sin, 0.0,
				0.0, -sin, cos, 0.0,
				0.0, 0.0, 0.0, 1.0
			);
		}

		static Matrix4x4 MakeRotateYMatrix(Radian radian)
		{
			float cos = Math::cos(radian);
			float sin = Math::sin(radian);

			return Matrix4x4(
				cos, 0.0, -sin, 0.0,
				0.0, 1.0, 0.0, 0.0,
				sin, 0.0, cos, 0.0,
				0.0, 0.0, 0.0, 1.0
			);
		}

		static Matrix4x4 MakeRotateZMatrix(Radian radian)
		{
			float cos = Math::cos(radian);
			float sin = Math::sin(radian);

			return Matrix4x4(
				cos, sin, 0.0, 0.0,
				-sin, cos, 0.0, 0.0,
				0.0, 0.0, 1.0, 0.0,
				0.0, 0.0, 0.0, 1.0
			);
		}

		static Matrix4x4 MakeRotateMatrix(const Vector3& axis)
		{
			return MakeRotateXMatrix(Radian(axis.x)) * MakeRotateYMatrix(Radian(axis.y)) * MakeRotateZMatrix(Radian(axis.z));
		}

		void Extract3x3Matrix(Matrix3x3& m3x3) const
		{
			m3x3.mat[0][0] = mat[0][0];
			m3x3.mat[0][1] = mat[0][1];
			m3x3.mat[0][2] = mat[0][2];
			m3x3.mat[1][0] = mat[1][0];
			m3x3.mat[1][1] = mat[1][1];
			m3x3.mat[1][2] = mat[1][2];
			m3x3.mat[2][0] = mat[2][0];
			m3x3.mat[2][1] = mat[2][1];
			m3x3.mat[2][2] = mat[2][2];
		}

		void ExtractAxes(Vector3& out_x, Vector3& out_y, Vector3& out_z) const
		{
			out_x = Vector3(mat[0][0], mat[1][0], mat[2][0]);
			out_x.Normalize();
			out_y = Vector3(mat[0][1], mat[1][1], mat[2][1]);
			out_y.Normalize();
			out_z = Vector3(mat[0][2], mat[1][2], mat[2][2]);
			out_z.Normalize();
		}

		bool HasScale() const
		{
			float t = mat[0][0] * mat[0][0] + mat[1][0] * mat[1][0] + mat[2][0] * mat[2][0];
			if (!Math::Equal(t, 1.0, (float)1e-04))
				return true;
			t = mat[0][1] * mat[0][1] + mat[1][1] * mat[1][1] + mat[2][1] * mat[2][1];
			if (!Math::Equal(t, 1.0, (float)1e-04))
				return true;
			t = mat[0][2] * mat[0][2] + mat[1][2] * mat[1][2] + mat[2][2] * mat[2][2];
			return !Math::Equal(t, 1.0, (float)1e-04);
		}

		bool HasNegativeScale() const { return Determinant() < 0; }

		Quaternion ExtractQuaternion() const
		{
			Matrix3x3 m3x3;
			Extract3x3Matrix(m3x3);
			return Quaternion(m3x3);
		}

		Matrix4x4 Adjoint() const;

		float Determinant() const
		{
			return mat[0][0] * GetMinor(1, 2, 3, 1, 2, 3) - mat[0][1] * GetMinor(1, 2, 3, 0, 2, 3) +
				mat[0][2] * GetMinor(1, 2, 3, 0, 1, 3) - mat[0][3] * GetMinor(1, 2, 3, 0, 1, 2);
		}

		void MakeAffine(const Vector3& scale, const Quaternion& orientation, const Vector3& position);

		void MakeAffine(const Vector3& scale, const Vector3& orientation, const Vector3& position);

		static Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& orientation, const Vector3& position);

		void MakeInverseTransform(const Vector3& scale, const Quaternion& orientation, const Vector3& position);

		void Decomposition(Vector3& scale, Quaternion& orientation, Vector3& position) const;

		void DecompositionWithoutScale(Vector3& position, Quaternion& rotation) const;

		bool IsAffine(void) const
		{
			return mat[3][0] == 0 && mat[3][1] == 0 && mat[3][2] == 0 && mat[3][3] == 1;
		}

		Matrix4x4 InverseAffine() const;

		Vector3 TransformAffine(const Vector3& v) const
		{
			assert(IsAffine());

			return Vector3(mat[0][0] * v.x + mat[0][1] * v.y + mat[0][2] * v.z + mat[0][3],
				mat[1][0] * v.x + mat[1][1] * v.y + mat[1][2] * v.z + mat[1][3],
				mat[2][0] * v.x + mat[2][1] * v.y + mat[2][2] * v.z + mat[2][3]);
		}

		Vector4 TransformAffine(const Vector4& v) const
		{
			assert(IsAffine());

			return Vector4(mat[0][0] * v.x + mat[0][1] * v.y + mat[0][2] * v.z + mat[0][3] * v.w,
				mat[1][0] * v.x + mat[1][1] * v.y + mat[1][2] * v.z + mat[1][3] * v.w,
				mat[2][0] * v.x + mat[2][1] * v.y + mat[2][2] * v.z + mat[2][3] * v.w,
				v.w);
		}

		Matrix4x4 Inverse() const
		{
			float m00 = mat[0][0], m01 = mat[0][1], m02 = mat[0][2], m03 = mat[0][3];
			float m10 = mat[1][0], m11 = mat[1][1], m12 = mat[1][2], m13 = mat[1][3];
			float m20 = mat[2][0], m21 = mat[2][1], m22 = mat[2][2], m23 = mat[2][3];
			float m30 = mat[3][0], m31 = mat[3][1], m32 = mat[3][2], m33 = mat[3][3];

			float v0 = m20 * m31 - m21 * m30;
			float v1 = m20 * m32 - m22 * m30;
			float v2 = m20 * m33 - m23 * m30;
			float v3 = m21 * m32 - m22 * m31;
			float v4 = m21 * m33 - m23 * m31;
			float v5 = m22 * m33 - m23 * m32;

			float t00 = +(v5 * m11 - v4 * m12 + v3 * m13);
			float t10 = -(v5 * m10 - v2 * m12 + v1 * m13);
			float t20 = +(v4 * m10 - v2 * m11 + v0 * m13);
			float t30 = -(v3 * m10 - v1 * m11 + v0 * m12);

			float invDet = 1 / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);

			float d00 = t00 * invDet;
			float d10 = t10 * invDet;
			float d20 = t20 * invDet;
			float d30 = t30 * invDet;

			float d01 = -(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
			float d11 = +(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
			float d21 = -(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
			float d31 = +(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

			v0 = m10 * m31 - m11 * m30;
			v1 = m10 * m32 - m12 * m30;
			v2 = m10 * m33 - m13 * m30;
			v3 = m11 * m32 - m12 * m31;
			v4 = m11 * m33 - m13 * m31;
			v5 = m12 * m33 - m13 * m32;

			float d02 = +(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
			float d12 = -(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
			float d22 = +(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
			float d32 = -(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

			v0 = m21 * m10 - m20 * m11;
			v1 = m22 * m10 - m20 * m12;
			v2 = m23 * m10 - m20 * m13;
			v3 = m22 * m11 - m21 * m12;
			v4 = m23 * m11 - m21 * m13;
			v5 = m23 * m12 - m22 * m13;

			float d03 = -(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
			float d13 = +(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
			float d23 = -(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
			float d33 = +(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

			return Matrix4x4(d00, d01, d02, d03, d10, d11, d12, d13, d20, d21, d22, d23, d30, d31, d32, d33);
		}

		const Vector3 GetX() const { return Vector3(mat[0]); }
		const Vector3 GetY() const { return Vector3(mat[1]); }
		const Vector3 GetZ() const { return Vector3(mat[2]); }
		const Vector3 GetW() const { return Vector3(mat[3]); }

		static Matrix4x4 MakeScale(const Vector3& scale)
		{
			Matrix4x4 m = Matrix4x4::IDENTITY;
			m.mat[0][0] = scale.x;
            m.mat[1][1] = scale.y;
            m.mat[2][2] = scale.z;
            return m;
		}

		Vector3 TransformCoord(const Vector3& v)const
		{
			Vector4 temp(v, 1.0f);
			Vector4 ret = (*this) * temp;
			if (ret.w == 0.0f)
			{
				return Vector3::ZERO;
			}
			else
			{
				ret /= ret.w;
				return Vector3(ret.x, ret.y, ret.z);
			}

			return Vector3::ZERO;
		}

		static const Matrix4x4 ZERO;
		static const Matrix4x4 ZEROAFFINE;
		static const Matrix4x4 IDENTITY;
	};

	Vector4 operator*(const Vector4& v, const Matrix4x4& mat);

	inline Vector3 Math::Transfrom(const Vector3& v, const Matrix4x4& mat)
	{
		Vector4 temp(v, 1.0f);
        return Vector3(temp * mat);
	}
	
	inline Vector3 Math::TransformNormal(const Vector3& v, const Matrix4x4& mat)
	{
		Vector4 temp(v, 0.0f);
        return Vector3(temp * mat).NormalizedCopy();
	}
};


