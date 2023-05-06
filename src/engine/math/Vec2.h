#pragma once
#pragma warning(disable:4201)

#include <Engine/Math/MathUtils.h>
#include <Engine/Core/Types.h>
#include <Engine/Core/Common.h>

#include <cmath>

class Vec2
{
public:
	enum { NumElements = 2 };

	union
	{
		struct
		{
			F32 m_x;
			F32 m_y;
		};

		F32 m_data[NumElements];
	};

	Vec2(){};
	inline Vec2(F32 x, F32 y);

	inline Vec2 operator*(F32 s) const;
	inline Vec2 operator/(F32 s) const;
	inline Vec2& operator*=(F32 s);
	inline Vec2& operator/=(F32 s);

	inline Vec2 operator+(const Vec2& v) const;
	inline Vec2& operator+=(const Vec2& v);
	inline Vec2 operator-(const Vec2& v) const;
	inline Vec2& operator-=(const Vec2& v);
	inline Vec2 operator-() const;

	inline bool operator==(const Vec2& other) const;

	inline F32 CalcLength() const;
	inline F32 CalcLengthSquared() const;

	inline void Normalize();
	inline Vec2 GetNormalized() const;

	static Vec2 ZERO;
};

inline Vec2 operator*(F32 s, const Vec2& v);
inline F32 Dot(const Vec2& a, const Vec2& b);
inline F32 Distance(const Vec2& a, const Vec2& b);
inline F32 DistanceSquared(const Vec2& a, const Vec2& b);

//////////////////////////////////////////////////////////////////////////

inline
Vec2::Vec2(F32 x, F32 y)
	:m_x(x)
	,m_y(y)
{
}

inline 
Vec2 Vec2::operator*(F32 s) const
{
	return Vec2(m_x * s, m_y * s);
}

inline 
Vec2 Vec2::operator/(F32 s) const
{
	F32 invS = 1.0f / s;
	return Vec2(m_x * invS, m_y * invS);
}

inline 
Vec2& Vec2::operator*=(F32 s)
{
	m_x *= s;
	m_y *= s;
	return *this;
}

inline 
Vec2& Vec2::operator/=(F32 s)
{
	F32 invS = 1.0f / s;
	m_x *= invS;
	m_y *= invS;
	return *this;
}

inline 
Vec2 Vec2::operator+(const Vec2& v) const
{
	return Vec2(m_x + v.m_x, m_y + v.m_y);
}

inline 
Vec2& Vec2::operator+=(const Vec2& v)
{
	m_x += v.m_x;
	m_y += v.m_y;
	return *this;
}

inline 
Vec2 Vec2::operator-(const Vec2& v) const
{
	return Vec2(m_x - v.m_x, m_y - v.m_y);
}

inline 
Vec2& Vec2::operator-=(const Vec2& v)
{
	m_x -= v.m_x;
	m_y -= v.m_y;
	return *this;
}

inline 
Vec2 Vec2::operator-() const
{
	return Vec2(-m_x, -m_y);
}

inline 
bool Vec2::operator==(const Vec2& other) const
{
	return m_x == other.m_x && m_y == other.m_y;
}

inline 
F32 Vec2::CalcLength() const
{
	return sqrtf(m_x * m_x + m_y * m_y);
}

inline 
F32 Vec2::CalcLengthSquared() const
{
	return m_x * m_x + m_y * m_y;
}

inline void Vec2::Normalize()
{
	// prevent division by zero by checking for zero length and asserting
	F32 lengthSq = CalcLengthSquared();
	ASSERT(!IsZero(lengthSq));
	if (IsZero(lengthSq))
	{
		m_x = m_y = 0.0f;
		return;
	}

	// do normalization now that we know length is not zero
	F32 length = CalcLength();
	F32 invLength = 1.0f / length;

	m_x *= invLength;
	m_y *= invLength;
}

inline Vec2 Vec2::GetNormalized() const
{
	Vec2 unit = *this;
	unit.Normalize();
	return unit;
}

inline 
Vec2 operator*(F32 s, const Vec2& v)
{
	return v * s;
}

inline
F32 Dot(const Vec2& a, const Vec2& b)
{
	return (a.m_x * b.m_x) + (a.m_y * b.m_y);
}

inline 
F32 Distance(const Vec2& a, const Vec2& b)
{
	Vec2 displacement = a - b;
	return displacement.CalcLength();
}

inline 
F32 DistanceSquared(const Vec2& a, const Vec2& b)
{
	Vec2 displacement = a - b;
	return displacement.CalcLengthSquared();
}