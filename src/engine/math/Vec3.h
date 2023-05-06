#pragma once
#pragma warning(disable:4201)

#include <engine/math/Vec2.h>

class Vec3
{
public:
	enum { NumElements = 3 };

	union
	{
		struct
		{
			F32 m_x;
			F32 m_y;
			F32 m_z;
		};

		struct
		{
			Vec2 m_xy;
			F32 m_pad;
		};

		F32 m_data[NumElements];
	};

	inline Vec3(){};
	inline Vec3(F32 x, F32 y, F32 z);

	inline Vec3 operator*(F32 s) const;
	inline Vec3 operator/(F32 s) const;
	inline Vec3& operator*=(F32 s);
	inline Vec3& operator/=(F32 s);

	inline Vec3 operator+(const Vec3& v) const;
	inline Vec3& operator+=(const Vec3& v);
	inline Vec3 operator-(const Vec3& v) const;
	inline Vec3& operator-=(const Vec3& v);
	inline Vec3 operator-() const;

	inline bool operator==(const Vec3& other) const;

	inline F32 CalcLength() const;
	inline F32 CalcLengthSquared() const;

	inline void Normalize();
	inline Vec3 GetNormalized() const;

	static Vec3 ZERO;
	static Vec3 FORWARD;
	static Vec3 BACKWARD;
	static Vec3 UP;
	static Vec3 DOWN;
	static Vec3 LEFT;
	static Vec3 RIGHT;
};

inline Vec3 operator*(F32 s, const Vec3& v);
inline F32 Dot(const Vec3& a, const Vec3& b);
inline Vec3 Cross(const Vec3& a, const Vec3& b);
inline F32 Distance(const Vec3& a, const Vec3& b);
inline F32 DistanceSquared(const Vec3& a, const Vec3& b);

inline
Vec3::Vec3(F32 x, F32 y, F32 z)
	:m_x(x)
	,m_y(y)
	,m_z(z)
{
}

inline 
Vec3 Vec3::operator*(F32 s) const
{
	return Vec3(m_x * s, m_y * s, m_z * s);
}

inline 
Vec3 Vec3::operator/(F32 s) const
{
	F32 invS = 1.0f / s;
	return Vec3(m_x * invS, m_y * invS, m_z * invS);
}

inline 
Vec3& Vec3::operator*=(F32 s)
{
	m_x *= s;
	m_y *= s;
	m_z *= s;
	return *this;
}

inline 
Vec3& Vec3::operator/=(F32 s)
{
	F32 invS = 1.0f / s;
	m_x *= invS;
	m_y *= invS;
	m_z *= invS;
	return *this;
}

inline 
Vec3 Vec3::operator+(const Vec3& v) const
{
	return Vec3(m_x + v.m_x, m_y + v.m_y, m_z + v.m_z);
}

inline 
Vec3& Vec3::operator+=(const Vec3& v)
{
	m_x += v.m_x;
	m_y += v.m_y;
	m_z += v.m_z;
	return *this;
}

inline 
Vec3 Vec3::operator-(const Vec3& v) const
{
	return Vec3(m_x - v.m_x, m_y - v.m_y, m_z - v.m_z);
}

inline 
Vec3& Vec3::operator-=(const Vec3& v)
{
	m_x -= v.m_x;
	m_y -= v.m_y;
	m_z -= v.m_z;
	return *this;
}

inline 
Vec3 Vec3::operator-() const
{
	return Vec3(-m_x, -m_y, -m_z);
}

inline 
bool Vec3::operator==(const Vec3& other) const
{
	return m_x == other.m_y && m_y == other.m_y && m_z == other.m_z;
}

inline 
F32 Vec3::CalcLength() const
{
	return sqrtf(m_x * m_x + m_y * m_y + m_z * m_z);
}

inline 
F32 Vec3::CalcLengthSquared() const
{
	return m_x * m_x + m_y * m_y + m_z * m_z;
}

inline void Vec3::Normalize()
{
	// prevent division by zero by checking for zero length and asserting
	F32 lengthSq = CalcLengthSquared();
	ASSERT(!IsZero(lengthSq));
	if (IsZero(lengthSq))
	{
		m_x = m_y = m_z = 0.0f;
		return;
	}

	// do normalization now that we know length is not zero
	F32 length = CalcLength();
	F32 invLength = 1.0f / length;

	m_x *= invLength;
	m_y *= invLength;
	m_z *= invLength;
}

inline Vec3 Vec3::GetNormalized() const
{
	Vec3 unit = *this;
	unit.Normalize();
	return unit;
}

inline 
Vec3 operator*(F32 s, const Vec3& v)
{
	return v * s;
}

inline
F32 Dot(const Vec3& a, const Vec3& b)
{
	return (a.m_x * b.m_x) + (a.m_y * b.m_y) + (a.m_z * b.m_z);
}

inline 
Vec3 Cross(const Vec3& a, const Vec3& b)
{
	Vec3 cross;
	cross.m_x = a.m_y * b.m_z - a.m_z * b.m_y;
	cross.m_y = a.m_z * b.m_x - a.m_x * b.m_z;
	cross.m_z = a.m_x * b.m_y - a.m_y * b.m_x;
	return cross;
}

inline 
F32 Distance(const Vec3& a, const Vec3& b)
{
	Vec3 displacement = a - b;
	return displacement.CalcLength();
}

inline 
F32 DistanceSquared(const Vec3& a, const Vec3& b)
{
	Vec3 displacement = a - b;
	return displacement.CalcLengthSquared();
}
