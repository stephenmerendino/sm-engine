#pragma once
#pragma warning(disable:4201)

#include "engine/core/Types.h"
#include "engine/math/MathUtils.h"

class IntVec2
{
public:
	enum { NumElements = 2 };

	union
	{
		struct
		{
			I32 m_x;
			I32 m_y;
		};

		I32 m_data[NumElements];
	};

	IntVec2() {};
	inline IntVec2(I32 x, I32 y);

	inline IntVec2 operator*(I32 s) const;
	inline IntVec2 operator/(I32 s) const;
	inline IntVec2& operator*=(I32 s);
	inline IntVec2& operator/=(I32 s);

	inline IntVec2 operator+(const IntVec2& v) const;
	inline IntVec2& operator+=(const IntVec2& v);
	inline IntVec2 operator-(const IntVec2& v) const;
	inline IntVec2& operator-=(const IntVec2& v);
	inline IntVec2 operator-() const;

	inline bool operator==(const IntVec2& other) const;

	inline F32 CalcLength() const;
	inline F32 CalcLengthSquared() const;

	static IntVec2 ZERO;
};

inline IntVec2 operator*(I32 s, const IntVec2& v);
inline F32 Dot(const IntVec2& a, const IntVec2& b);
inline F32 Distance(const IntVec2& a, const IntVec2& b);
inline F32 DistanceSquared(const IntVec2& a, const IntVec2& b);

inline 
IntVec2::IntVec2(I32 x, I32 y)
	:m_x(x)
	,m_y(y)
{
}

inline 
IntVec2 IntVec2::operator*(I32 s) const
{
	return IntVec2(m_x * s, m_y * s);
}

inline 
IntVec2 IntVec2::operator/(I32 s) const
{
	return IntVec2(m_x / s, m_y / s);
}

inline 
IntVec2& IntVec2::operator*=(I32 s)
{
	m_x *= s;
	m_y *= s;
	return *this;
}

inline 
IntVec2& IntVec2::operator/=(I32 s)
{
	m_x /= s;
	m_y /= s;
	return *this;
}

inline 
IntVec2 IntVec2::operator+(const IntVec2& v) const
{
	return IntVec2(m_x + v.m_x, m_y + v.m_y);
}

inline 
IntVec2& IntVec2::operator+=(const IntVec2& v)
{
	m_x += v.m_x;
	m_y += v.m_y;
	return *this;
}

inline 
IntVec2 IntVec2::operator-(const IntVec2& v) const
{
	return IntVec2(m_x - v.m_x, m_y - v.m_y);
}

inline 
IntVec2& IntVec2::operator-=(const IntVec2& v)
{
	m_x -= v.m_x;
	m_y -= v.m_y;
	return *this;
}

inline 
IntVec2 IntVec2::operator-() const
{
	return IntVec2(-m_x, -m_y);
}

inline 
bool IntVec2::operator==(const IntVec2& other) const
{
	return m_x == other.m_x && m_y == other.m_y;
}

inline 
F32 IntVec2::CalcLength() const
{
	return sqrt(CalcLengthSquared());
}

inline 
F32 IntVec2::CalcLengthSquared() const 
{
	return (F32)(m_x * m_x + m_y * m_y);
}

inline 
IntVec2 operator*(I32 s, const IntVec2& v)
{
	return v * s;
}

inline 
F32 Dot(const IntVec2& a, const IntVec2& b)
{
	return (F32)((a.m_x * b.m_x) + (a.m_y * b.m_y));
}

inline 
F32 Distance(const IntVec2& a, const IntVec2& b)
{
	IntVec2 displacement = a - b;
	return displacement.CalcLength();
}

inline 
F32 DistanceSquared(const IntVec2& a, const IntVec2& b)
{
	IntVec2 displacement = a - b;
	return displacement.CalcLengthSquared();
}
