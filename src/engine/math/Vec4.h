#pragma once
#pragma warning(disable:4201)

#include "engine/math/Vec3.h"

class Vec4
{
public:
	enum { NumElements = 4 };

	union
	{
		struct
		{
			F32 m_x;
			F32 m_y;
			F32 m_z;
			F32 m_w;
		};

		struct
		{
			Vec3 m_xyz;
			F32 m_pad;
		};

		F32 m_data[NumElements];
	};


	inline Vec4(){};
	inline Vec4(F32 x, F32 y, F32 z, F32 w);
	inline Vec4(const Vec3& xyz, F32 w);

	inline Vec4 operator*(F32 s) const;
	inline Vec4 operator/(F32 s) const;
	inline Vec4& operator*=(F32 s);
	inline Vec4& operator/=(F32 s);

	inline Vec4 operator+(const Vec4& v) const;
	inline Vec4& operator+=(const Vec4& v);
	inline Vec4 operator-(const Vec4& v) const;
	inline Vec4& operator-=(const Vec4& v);
	inline Vec4 operator-() const;

	inline bool operator==(const Vec4& other) const;

	inline F32 CalcLength() const;
	inline F32 CalcLengthSquared() const;

	inline void Normalize();
	inline Vec4 GetNormalized() const;

	static Vec4 ZERO;
};

inline Vec4 operator*(F32 s, const Vec4& v);
inline F32 Dot(const Vec4& a, const Vec4& b);
inline F32 Distance(const Vec4& a, const Vec4& b);
inline F32 DistanceSquared(const Vec4& a, const Vec4& b);

inline
Vec4::Vec4(F32 x, F32 y, F32 z, F32 w)
	:m_x(x)
	,m_y(y)
	,m_z(z)
	,m_w(w)
{
}

inline 
Vec4::Vec4(const Vec3& xyz, F32 w)
	:m_x(xyz.m_x)
	,m_y(xyz.m_y)
	,m_z(xyz.m_z)
	,m_w(w)
{
}

inline 
Vec4 Vec4::operator*(F32 s) const
{
	return Vec4(m_x * s, m_y * s, m_z * s, m_w * s);
}

inline 
Vec4 Vec4::operator/(F32 s) const
{
	F32 invS = 1.0f / s;
	return Vec4(m_x * invS, m_y * invS, m_z * invS, m_w * invS);
}

inline 
Vec4& Vec4::operator*=(F32 s)
{
	m_x *= s;
	m_y *= s;
	m_z *= s;
	m_w *= s;
	return *this;
}

inline 
Vec4& Vec4::operator/=(F32 s)
{
	F32 invS = 1.0f / s;
	m_x *= invS;
	m_y *= invS;
	m_z *= invS;
	m_w *= invS;
	return *this;
}

inline 
Vec4 Vec4::operator+(const Vec4& v) const
{
	return Vec4(m_x + v.m_x, m_y + v.m_y, m_z + v.m_z, m_w + v.m_w);
}

inline 
Vec4& Vec4::operator+=(const Vec4& v)
{
	m_x += v.m_x;
	m_y += v.m_y;
	m_z += v.m_z;
	m_w += v.m_w;
	return *this;
}

inline 
Vec4 Vec4::operator-(const Vec4& v) const
{
	return Vec4(m_x - v.m_x, m_y - v.m_y, m_z - v.m_z, m_w - v.m_w);
}

inline 
Vec4& Vec4::operator-=(const Vec4& v)
{
	m_x -= v.m_x;
	m_y -= v.m_y;
	m_z -= v.m_z;
	m_w -= v.m_w;
	return *this;
}

inline 
Vec4 Vec4::operator-() const
{
	return Vec4(-m_x, -m_y, -m_z, -m_w);
}

inline 
bool Vec4::operator==(const Vec4& other) const
{
	return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z && m_w == other.m_w;
}

inline 
F32 Vec4::CalcLength() const
{
	return sqrtf(m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w);
}

inline 
F32 Vec4::CalcLengthSquared() const
{
	return m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w;
}

inline void Vec4::Normalize()
{
	// prevent division by zero by checking for zero length and asserting
	F32 lengthSq = CalcLengthSquared();
	ASSERT(!IsZero(lengthSq));
	if (IsZero(lengthSq))
	{
		m_x = m_y = m_z = m_w = 0.0f;
		return;
	}

	// do normalization now that we know length is not zero
	F32 length = CalcLength();
	F32 invLength = 1.0f / length;

	m_x *= invLength;
	m_y *= invLength;
	m_z *= invLength;
	m_w *= invLength;
}

inline Vec4 Vec4::GetNormalized() const
{
	Vec4 unit = *this;
	unit.Normalize();
	return unit;
}

inline 
Vec4 operator*(F32 s, const Vec4& v)
{
	return v * s;
}

inline
F32 Dot(const Vec4& a, const Vec4& b)
{
	return (a.m_x * b.m_x) + (a.m_y * b.m_y) + (a.m_z * b.m_z) + (a.m_w * b.m_w);
}

inline 
F32 Distance(const Vec4& a, const Vec4& b)
{
	Vec4 displacement = a - b;
	return displacement.CalcLength();
}

inline 
F32 DistanceSquared(const Vec4& a, const Vec4& b)
{
	Vec4 displacement = a - b;
	return displacement.CalcLengthSquared();
}
