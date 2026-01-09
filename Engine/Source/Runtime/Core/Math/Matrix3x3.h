#pragma once
#include "Math.h"
#include "Vector3.h"
#include "Quaternion.h"

namespace AtomEngine
{
	__declspec(align(16))class Matrix3x3
	{
	public:
		float mat[3][3];

		Matrix3x3() { operator=(IDENTITY); }
		Matrix3x3(const Matrix4x4& m);
		explicit Matrix3x3(float arr[3][3])
		{
			memcpy(mat[0], arr[0], 3 * sizeof(float));
			memcpy(mat[1], arr[1], 3 * sizeof(float));
			memcpy(mat[2], arr[2], 3 * sizeof(float));
		}

		Matrix3x3(float(&fArray)[9])
		{
			mat[0][0] = fArray[0];
			mat[0][1] = fArray[1];
			mat[0][2] = fArray[2];
			mat[1][0] = fArray[3];
			mat[1][1] = fArray[4];
			mat[1][2] = fArray[5];
			mat[2][0] = fArray[6];
			mat[2][1] = fArray[7];
			mat[2][2] = fArray[8];
		}

		Matrix3x3(float entry00,
			float entry01,
			float entry02,
			float entry10,
			float entry11,
			float entry12,
			float entry20,
			float entry21,
			float entry22)
		{
			mat[0][0] = entry00;
			mat[0][1] = entry01;
			mat[0][2] = entry02;
			mat[1][0] = entry10;
			mat[1][1] = entry11;
			mat[1][2] = entry12;
			mat[2][0] = entry20;
			mat[2][1] = entry21;
			mat[2][2] = entry22;
		}
		Matrix3x3(const Vector3& row0, const Vector3& row1, const Vector3& row2)
		{
			mat[0][0] = row0.x;
			mat[0][1] = row0.y;
			mat[0][2] = row0.z;
			mat[1][0] = row1.x;
			mat[1][1] = row1.y;
			mat[1][2] = row1.z;
			mat[2][0] = row2.x;
			mat[2][1] = row2.y;
			mat[2][2] = row2.z;
		}

		Matrix3x3(const Quaternion& q);

		Vector3 GetColumn(int col) const
		{
			assert(0 <= col && col < 3);
			return Vector3(mat[0][col], mat[1][col], mat[2][col]);
		}

		Quaternion GetRotation() const
		{
			return Quaternion(*this);
		}

		const Vector3 GetX() const { return Vector3(mat[0]); }
		const Vector3 GetY() const { return Vector3(mat[1]); }
		const Vector3 GetZ() const { return Vector3(mat[2]); }

		bool operator==(const Matrix3x3& rhs) const
		{
			for (size_t row = 0; row < 3; row++)
			{
				for (size_t col = 0; col < 3; col++)
				{
					if (mat[row][col] != rhs.mat[row][col])
						return false;
				}
			}

			return true;
		}

		bool operator!=(const Matrix3x3& rhs) const { return !operator==(rhs); }

		Matrix3x3 operator+(const Matrix3x3& rhs) const
		{
			Matrix3x3 sum;
			for (size_t row = 0; row < 3; row++)
			{
				for (size_t col = 0; col < 3; col++)
				{
					sum.mat[row][col] = mat[row][col] + rhs.mat[row][col];
				}
			}
			return sum;
		}

		Matrix3x3 operator-(const Matrix3x3& rhs) const
		{
			Matrix3x3 diff;
			for (size_t row = 0; row < 3; row++)
			{
				for (size_t col = 0; col < 3; col++)
				{
					diff.mat[row][col] = mat[row][col] - rhs.mat[row][col];
				}
			}
			return diff;
		}

        Matrix3x3 operator*(const Matrix3x3& rhs) const
		{
			Matrix3x3 result;
			for (size_t row = 0; row < 3; row++)
			{
                for (size_t col = 0; col < 3; col++)
				{
					result.mat[row][col] =	mat[row][0] * rhs.mat[0][col] + 
											mat[row][1] * rhs.mat[1][col] + 
											mat[row][2] * rhs.mat[2][col];
				}
			}
			return result;
		}

		Vector3 operator*(const Vector3& rhs) const
		{
			Vector3 prod;
			for (size_t row = 0; row < 3; row++)
			{
				prod[row] =
					mat[row][0] * rhs.x + mat[row][1] * rhs.y + mat[row][2] * rhs.z;
			}
			return prod;
		}

		friend Vector3 operator*(const Vector3& point, const Matrix3x3& rhs)
		{
			Vector3 prod;
			for (size_t row = 0; row < 3; row++)
			{
				prod[row] = point.x * rhs.mat[0][row] + point.y * rhs.mat[1][row] +
					point.z * rhs.mat[2][row];
			}
			return prod;
		}

		Matrix3x3 operator-() const
		{
			Matrix3x3 neg;
			for (size_t row = 0; row < 3; row++)
			{
				for (size_t col = 0; col < 3; col++)
					neg.mat[row][col] = -mat[row][col];
			}
			return neg;
		}

		// matrix * scalar
		Matrix3x3 operator*(float scalar) const
		{
			Matrix3x3 prod;
			for (size_t row = 0; row < 3; row++)
			{
				for (size_t col = 0; col < 3; col++)
					prod.mat[row][col] = scalar * mat[row][col];
			}
			return prod;
		}

		// scalar * matrix
		friend Matrix3x3 operator*(float scalar, const Matrix3x3& rhs)
		{
			Matrix3x3 prod;
			for (size_t row = 0; row < 3; row++)
			{
				for (size_t col = 0; col < 3; col++)
					prod.mat[row][col] = scalar * rhs.mat[row][col];
			}
			return prod;
		}

		// utilities
		Matrix3x3 Transpose() const
		{
			Matrix3x3 transpose;
			for (size_t row = 0; row < 3; row++)
			{
				for (size_t col = 0; col < 3; col++)
					transpose.mat[row][col] = mat[col][row];
			}
			return transpose;
		}

		bool Inverse(Matrix3x3& invMat, float fTolerance = 1e-06) const
		{
			float det = Determinant();
			if (std::fabs(det) <= fTolerance)
				return false;

			invMat.mat[0][0] = mat[1][1] * mat[2][2] - mat[1][2] * mat[2][1];
			invMat.mat[0][1] = mat[0][2] * mat[2][1] - mat[0][1] * mat[2][2];
			invMat.mat[0][2] = mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1];
			invMat.mat[1][0] = mat[1][2] * mat[2][0] - mat[1][0] * mat[2][2];
			invMat.mat[1][1] = mat[0][0] * mat[2][2] - mat[0][2] * mat[2][0];
			invMat.mat[1][2] = mat[0][2] * mat[1][0] - mat[0][0] * mat[1][2];
			invMat.mat[2][0] = mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0];
			invMat.mat[2][1] = mat[0][1] * mat[2][0] - mat[0][0] * mat[2][1];
			invMat.mat[2][2] = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];

			float inv_det = 1.0f / det;
			for (size_t row = 0; row < 3; row++)
			{
				for (size_t col = 0; col < 3; col++)
					invMat.mat[row][col] *= inv_det;
			}

			return true;
		}

		Matrix3x3 Inverse(float tolerance = 1e-06) const
		{
			Matrix3x3 inv = ZERO;
			Inverse(inv, tolerance);
			return inv;
		}

		// デルタ(det|A|)
		float Determinant() const
		{
			return mat[0][0] * mat[1][1] * mat[2][2] +
				mat[0][1] * mat[1][2] * mat[2][0] +
				mat[0][2] * mat[1][0] * mat[2][1] -
				mat[0][2] * mat[1][1] * mat[2][0] -
				mat[0][1] * mat[1][0] * mat[2][2] -
				mat[0][0] * mat[1][2] * mat[2][1];
		}

		static Matrix3x3 MakeScale(const Vector3& v);

		static const Matrix3x3 ZERO;
		static const Matrix3x3 IDENTITY;
	};
	inline Matrix3x3 Math::InverseTranspose(const Matrix3x3& mat)
	{
		const Vector3 x = mat.GetX();
		const Vector3 y = mat.GetY();
		const Vector3 z = mat.GetZ();

		const Vector3 inv0 = Cross(y, z);
		const Vector3 inv1 = Cross(z, x);
		const Vector3 inv2 = Cross(x, y);
		const float rDet = 1.0f / (Dot(z, inv2));

		return Matrix3x3(inv0, inv1, inv2) * rDet;
	}
}
