#pragma once
#pragma warning(disable:4201)

#include "Engine/Math/MathUtils.h"
#include "Engine/Core/Types.h"
#include "Engine/Core/Assert.h"

#include <cmath>

class Vec2
{
public:
	inline Vec2();
	inline Vec2(F32 inX, F32 inY);

	inline Vec2 operator*(F32 s) const;
	inline Vec2 operator/(F32 s) const;
	inline Vec2& operator*=(F32 s);
	inline Vec2& operator/=(F32 s);
	inline Vec2 operator+(const Vec2& other) const;
	inline Vec2 operator-(const Vec2& other) const;
	inline Vec2& operator+=(const Vec2& other);
	inline Vec2& operator-=(const Vec2& other);
	inline Vec2 operator-() const;
	inline bool operator==(const Vec2& other) const;

	inline F32 CalcLengthSq() const;
	inline F32 CalcLength() const;
	inline void Normalize();
	inline Vec2 Normalized() const;

	union
	{
		struct
		{
			F32 x;
			F32 y;
		};

		F32 data[2];
	};

	static const Vec2 ZERO;
};

inline Vec2::Vec2()
	:x(0)
	,y(0)
{
}

inline Vec2::Vec2(F32 inX, F32 inY)
	:x(inX)
	,y(inY)
{
}

inline Vec2 Vec2::operator*(F32 s) const
{
	return Vec2(x * s, y * s);
}

inline Vec2 Vec2::operator/(F32 s) const
{
	return Vec2(x * s, y * s);
}

inline Vec2& Vec2::operator*=(F32 s)
{
	x *= s;
	y *= s;
	return *this;
}

inline Vec2& Vec2::operator/=(F32 s)
{
	x *= s;
	y *= s;
	return *this;
}

inline Vec2 Vec2::operator+(const Vec2& other) const
{
	return Vec2(x + other.x, y + other.y);
}

inline Vec2 Vec2::operator-(const Vec2& other) const
{
	return Vec2(x - other.x, y - other.y);
}

inline Vec2& Vec2::operator+=(const Vec2& other)
{
	x += other.x;
	y += other.y;
	return *this;
}

inline Vec2& Vec2::operator-=(const Vec2& other)
{
	x -= other.x;
	y -= other.y;
	return *this;
}

inline Vec2 Vec2::operator-() const
{
	return Vec2(-x, -y);
}

inline bool Vec2::operator==(const Vec2& other) const
{
	return (x == other.x) && (y == other.y);
}

inline F32 Vec2::CalcLengthSq() const
{
	return (x * x) + (y * y);
}

inline F32 Vec2::CalcLength() const
{
	return sqrtf(CalcLengthSq());
}

inline void Vec2::Normalize()
{
	// prevent division by zero by checking for zero length and asserting
	F32 lengthSq = CalcLengthSq();
	SM_ASSERT(!IsZero(lengthSq));

	// do normalization now that we know length is not zero
	F32 length = sqrtf(lengthSq);
	F32 invLength = 1.0f / length;

	*this *= invLength;
}

inline Vec2 Vec2::Normalized() const
{
	Vec2 copy = *this;
	copy.Normalize();
	return copy;
}

inline Vec2 operator*(F32 s, const Vec2& v)
{
	return v * s;
}

inline F32 Dot(const Vec2& a, const Vec2& b)
{
	return (a.x * b.x) + (a.y * b.y);
}

inline F32 Distance(const Vec2& a, const Vec2& b)
{
	return (a - b).CalcLength();
}

inline F32 DistanceSq(const Vec2& a, const Vec2& b)
{
	return (a - b).CalcLengthSq();
}

inline Vec2 PolarToCartesianRads(F32 radians, F32 radius = 1.0f)
{
	return radius * Vec2(cosf(radians), sinf(radians));
}

inline Vec2 PolarToCartesianDegs(F32 deg, F32 radius = 1.0f)
{
	return radius * Vec2(CosDeg(deg), SinDeg(deg));
}
