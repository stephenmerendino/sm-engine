#pragma once
#pragma warning(disable:4201)

#include "Engine/Core/Assert_old.h"
#include "Engine/Core/Types.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/Vec3_old.h"

class Vec4
{
public:
	inline Vec4();
	inline Vec4(F32 inX, F32 inY, F32 inZ, F32 inW);
	inline Vec4(const Vec3& xyz, F32 inW);

	inline Vec4 operator*(F32 s) const;
	inline Vec4 operator/(F32 s) const;
	inline Vec4& operator*=(F32 s);
	inline Vec4& operator/=(F32 s);
	inline Vec4 operator+(const Vec4& other) const;
	inline Vec4 operator-(const Vec4& other) const;
	inline Vec4& operator+=(const Vec4& other);
	inline Vec4& operator-=(const Vec4& other);
	inline Vec4 operator-() const;
	inline bool operator==(const Vec4& other) const;

	inline F32 CalcLengthSq() const;
	inline F32 CalcLength() const;
	inline void Normalize();
	inline Vec4 GetNormalized() const;

	union
	{
		struct
		{
			F32 x;
			F32 y;
			F32 z;
			F32 w;
		};

		struct
		{
			Vec3 xyz;
			F32 pad0;
		};

		F32 data[4];
	};

	static const Vec4 ZERO;
	static const Vec4 FORWARD;
	static const Vec4 BACKWARD;
	static const Vec4 UP;
	static const Vec4 DOWN;
	static const Vec4 LEFT;
	static const Vec4 RIGHT;
};

inline Vec4::Vec4()
	:x(0)
	,y(0)
	,z(0)
	,w(0)
{
}

inline Vec4::Vec4(F32 inX, F32 inY, F32 inZ, F32 inW)
	:x(inX)
	,y(inY)
	,z(inZ)
	,w(inW)
{
}

inline Vec4::Vec4(const Vec3& inXYZ, F32 inW)
{
	// can't initializer list members from 2 different unions so just do it in the body
	xyz = inXYZ;
	w = inW;
}

inline Vec4 Vec4::operator*(F32 s) const
{
	return Vec4(x * s, y * s, z * s, w * s);
}

inline Vec4 Vec4::operator/(F32 s) const
{
	F32 invS = 1.0f / s;
	return Vec4(x * invS, y * invS, z * invS, w * invS);
}

inline Vec4& Vec4::operator*=(F32 s)
{
	x *= s;
	y *= s;
	z *= s;
	w *= s;
	return *this;
}

inline Vec4& Vec4::operator/=(F32 s)
{
	F32 invS = 1.0f / s;
	x *= invS;
	y *= invS;
	z *= invS;
	w *= invS;
	return *this;
}

inline Vec4 Vec4::operator+(const Vec4& other) const
{
	return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
}

inline Vec4 Vec4::operator-(const Vec4& other) const
{
	return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
}

inline Vec4& Vec4::operator+=(const Vec4& other)
{
	x += other.x;
	y += other.y;
	z += other.z;
	w += other.w;
	return *this;
}

inline Vec4& Vec4::operator-=(const Vec4& other)
{
	x -= other.x;
	y -= other.y;
	z -= other.z;
	w -= other.w;
	return *this;
}

inline Vec4 Vec4::operator-() const
{
	return Vec4(-x, -y, -z, -w);
}

inline bool Vec4::operator==(const Vec4& other) const
{
	return (x == other.x) && (y == other.y) && (z == other.z) && (w == other.w);
}

inline F32 Vec4::CalcLengthSq() const
{
	return (x * x) + (y * y) + (z * z) + (w * w);
}

inline F32 Vec4::CalcLength() const
{
	return sqrtf(CalcLengthSq());
}

inline void Vec4::Normalize()
{
	// prevent division by zero by checking for zero length and asserting
	F32 lengthSq = CalcLengthSq();
	SM_ASSERT_OLD(!IsZero(lengthSq));

	// do normalization now that we know length is not zero
	F32 length = sqrtf(lengthSq);
	F32 invLength = 1.0f / length;

	*this *= invLength;
}

inline Vec4 Vec4::GetNormalized() const
{
	Vec4 copy = *this;
	copy.Normalize();
	return copy;
}

inline Vec4 operator*(F32 s, const Vec4& v)
{
	return v * s;
}

inline F32 Dot(const Vec4& a, const Vec4& b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}

inline F32 Distance(const Vec4& a, const Vec4& b)
{
	return (a - b).CalcLength();
}

inline F32 DistanceSq(const Vec4& a, const Vec4& b)
{
	return (a - b).CalcLengthSq();
}
