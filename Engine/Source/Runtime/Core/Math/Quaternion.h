#pragma once
#include "Math.h"

namespace AtomEngine
{
	class Quaternion
	{
	public:
		float x, y, z, w;
		Quaternion() = default;
		Quaternion(float x_, float y_, float z_, float w_) : x{ x_ }, y{ y_ }, z{ z_ }, w{ w_ } {}

		explicit Quaternion(const Matrix3x3& rot) { this->FromRotationMatrix(rot); }
		explicit Quaternion(const Matrix4x4& rot) { this->FromRotationMatrix(rot); }

		Quaternion(const Radian& angle, const Vector3& axis) { this->FromAngleAxis(angle, axis); }
		Quaternion(const Vector3& xaxis, const Vector3& yaxis, const Vector3& zaxis)
		{
			this->FromAxes(xaxis, yaxis, zaxis);
		}
		float* ptr() { return &x; }

		const float* ptr() const { return &x; }

		void FromRotationMatrix(const Matrix3x3& rotation);
        void FromRotationMatrix(const Matrix4x4& rotation);
		void ToRotationMatrix(Matrix3x3& rotation) const;
		void ToRotationMatrix(Matrix4x4& rotation) const;

		void FromAngleAxis(const Radian& angle, const Vector3& axis);

		static Quaternion GetQuaternionFromAngleAxis(const Radian& angle, const Vector3& axis);

		void FromDirection(const Vector3& direction, const Vector3& upDir);

		static Quaternion GetQuaternionFromDirection(const Vector3& direction, const Vector3& upDir);

		void ToAngleAxis(Radian& angle, Vector3& axis) const;


		void FromAxes(const Vector3& x_axis, const Vector3& y_axis, const Vector3& z_axis);

		void ToAxes(Vector3& x_axis, Vector3& y_axis, Vector3& z_axis) const;


		Vector3 xAxis() const;

		Vector3 yAxis() const;

		Vector3 zAxis() const;

		Quaternion operator+(const Quaternion& rhs) const
		{
			return Quaternion(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
		}

		Quaternion operator-(const Quaternion& rhs) const
		{
			return Quaternion(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
		}

		Quaternion Mul(const Quaternion& rhs) const { return (*this) * rhs; }
		Quaternion operator*(const Quaternion& rhs) const;

		Quaternion operator*(float scalar) const { return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar); }

		Vector3 operator*(const Vector3& rhs) const;

		Quaternion operator/(float scalar) const
		{
			assert(scalar != 0.0f);
			return Quaternion(x / scalar, y / scalar, z / scalar, w / scalar);
		}

		friend Quaternion operator*(float scalar, const Quaternion& rhs)
		{
			return Quaternion(scalar * rhs.x, scalar * rhs.y, scalar * rhs.z, scalar * rhs.w);
		}

		Quaternion operator-() const { return Quaternion(-x, -y, -z, -w); }

		bool operator==(const Quaternion& rhs) const
		{
			return (rhs.x == x) && (rhs.y == y) && (rhs.z == z) && (rhs.w == w);
		}

		bool operator!=(const Quaternion& rhs) const
		{
			return (rhs.x != x) || (rhs.y != y) || (rhs.z != z) || (rhs.w != w);
		}

		bool IsNaN() const { return Math::IsNaN(x) || Math::IsNaN(y) || Math::IsNaN(z) || Math::IsNaN(w); }

		float Dot(const Quaternion& rkQ) const { return x * rkQ.x + y * rkQ.y + z * rkQ.z + w * rkQ.w; }

		float Length() const { return std::sqrt( x * x + y * y + z * z + w * w); }

		void Normalize(void)
		{
			float factor = 1.0f / this->Length();
			*this = *this * factor;
		}

		Quaternion NormalizeCopy() const 
		{ 
			Quaternion q = *this; 
			q.Normalize(); 
			return q; 
		}

		Quaternion Inverse() const
		{
			float norm = x * x + y * y + z * z + w * w;
			if (norm > 0.0)
			{
				float inv = 1.0f / norm;
				return Quaternion(-x * inv, -y * inv, -z * inv, w * inv);
			}
			else
			{
				return ZERO;
			}
		}

		Radian Roll(bool reprojectAxis = true) const;

		Radian Pitch(bool reprojectAxis = true) const;

		Radian Yaw(bool reprojectAxis = true) const;

		/// <summary>
		/// 共役
		/// </summary>
		/// <returns>四元数</returns>
		Quaternion Conjugate() const { return Quaternion(-x, -y, -z, w); }

		static const Quaternion ZERO;
		static const Quaternion IDENTITY;
	};

	Quaternion Slerp(const Quaternion& kp, const Quaternion& kq, float t, bool shortestPath = false);

	Quaternion Lerp(const Quaternion& kp, const Quaternion& kq, float t, bool shortestPath = false);

	template <typename T>
	inline Quaternion ToQuat(const T* rot)
	{
		return Quaternion(ToFloat(rot[0]), ToFloat(rot[1]), ToFloat(rot[2]), ToFloat(rot[3]));
	}

	inline Quaternion ToQuat(const float* rot)
	{
		return Quaternion(rot[0], rot[1], rot[2], rot[3]);
	}
}


