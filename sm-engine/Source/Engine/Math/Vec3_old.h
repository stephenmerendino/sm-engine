#pragma once
#pragma warning(disable:4201)

#include <Engine/math/Vec2_old.h>

class Vec3
{
public:
	inline Vec3();
	inline Vec3(F32 inX, F32 inY, F32 inZ);
	inline Vec3(const Vec2& inXY, F32 inZ);

	inline Vec3 operator*(F32 s) const;
	inline Vec3 operator/(F32 s) const;
	inline Vec3& operator*=(F32 s);
	inline Vec3& operator/=(F32 s);
	inline Vec3 operator+(const Vec3& other) const;
	inline Vec3& operator+=(const Vec3& other);
	inline Vec3 operator-(const Vec3& other) const;
	inline Vec3& operator-=(const Vec3& other);
	inline Vec3 operator-() const;
	inline bool operator==(const Vec3& other);

	inline F32 CalcLengthSq() const;
	inline F32 CalcLength() const;
	inline void Normalize();
	inline Vec3 Normalized() const;

	union
	{
		struct
		{
			F32 x;
			F32 y;
			F32 z;
		};

		F32 data[3];
	};

	static const Vec3 ZERO;
	static const Vec3 FORWARD;
	static const Vec3 BACKWARD;
	static const Vec3 UP;
	static const Vec3 DOWN;
	static const Vec3 LEFT;
	static const Vec3 RIGHT;
};

inline Vec3::Vec3()
	:x(0)
	,y(0)
	,z(0)
{
}

inline Vec3::Vec3(F32 inX, F32 inY, F32 inZ)
	:x(inX)
	,y(inY)
	,z(inZ)
{
}

inline Vec3::Vec3(const Vec2& inXY, F32 inZ)
	:x(inXY.x)
	,y(inXY.y)
	,z(inZ)
{
}

inline Vec3 Vec3::operator*(F32 s) const
{
	return Vec3(x * s, y * s, z * s);
}

inline Vec3 Vec3::operator/(F32 s) const
{
	F32 invS = 1.0f / s;
	return Vec3(x * invS, y * invS, z * invS);
}

inline Vec3& Vec3::operator*=(F32 s)
{
	x *= s;
	y *= s;
	z *= s;
	return *this;
}

inline Vec3& Vec3::operator/=(F32 s)
{
	F32 invS = 1.0f / s;
	x *= invS;
	y *= invS;
	z *= invS;
	return *this;
}

inline Vec3 Vec3::operator+(const Vec3& other) const
{
	return Vec3(x + other.x, y + other.y, z + other.z);
}

inline Vec3& Vec3::operator+=(const Vec3& other)
{
	x += other.x;
	y += other.y;
	z += other.z;
	return *this;
}

inline Vec3 Vec3::operator-(const Vec3& other) const
{
	return Vec3(x - other.x, y - other.y, z - other.z);
}

inline Vec3& Vec3::operator-=(const Vec3& other)
{
	x -= other.x;
	y -= other.y;
	z -= other.z;
	return *this;
}

inline Vec3 Vec3::operator-() const
{
	return Vec3(-x, -y, -z);
}

inline bool Vec3::operator==(const Vec3& other)
{
	return (x == other.x) && (y == other.y) && (z == other.z);
}

inline F32 Vec3::CalcLengthSq() const
{
	return (x * x) + (y * y) + (z * z);
}

inline F32 Vec3::CalcLength() const
{
	return sqrtf(CalcLengthSq());
}

inline void Vec3::Normalize()
{
	// prevent division by zero by checking for zero length and asserting
	F32 lengthSq = CalcLengthSq();
	SM_ASSERT_OLD(!IsZero(lengthSq));

	// do normalization now that we know length is not zero
	F32 length = sqrtf(lengthSq);
	F32 invLength = 1.0f / length;

	*this *= invLength;
}

inline Vec3 Vec3::Normalized() const
{
	Vec3 copy = *this;
	copy.Normalize();
	return copy;
}

inline Vec3 operator*(F32 s, const Vec3& v)
{
	return v * s;
}

inline F32 Dot(const Vec3& a, const Vec3& b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

inline Vec3 Cross(const Vec3& a, const Vec3& b)
{
	Vec3 cross;
	cross.x = a.y * b.z - a.z * b.y;
	cross.y = a.z * b.x - a.x * b.z;
	cross.z = a.x * b.y - a.y * b.x;
	return cross;
}

inline F32 distance(const Vec3& a, const Vec3& b)
{
	return (a - b).CalcLength();
}

inline F32 DistanceSquared(const Vec3& a, const Vec3& b)
{
	return (a - b).CalcLengthSq();
}
