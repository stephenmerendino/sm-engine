#pragma once
#pragma warning(disable:4201)

#include "Engine/Core/Assert.h"
#include "Engine/Core/Types.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/Vec3.h"

class Vec4
{
public:
	inline Vec4();
	inline Vec4(F32 inX, F32 inY, F32 inZ, F32 inW);

	inline Vec4 operator*(F32 s) const;
	inline Vec4 operator/(F32 s) const;
	inline Vec4& operator*=(F32 s);
	inline Vec4& operator/=(F32 s);
	inline Vec4 operator+(const Vec4& other) const;
	inline Vec4 operator-(const Vec4& other) const;
	inline Vec4& operator+=(const Vec4& other);
	inline Vec4& operator-=(const Vec4& other);

	union
	{
		struct
		{
			F32 x;
			F32 y;
			F32 z;
			F32 w;
		};

		F32 data[4];
	};
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

inline
vec4 operator-(const vec4& v)
{
	return make_vec4(-v.x, -v.y, -v.z, -v.w);
}

inline
bool operator==(const vec4& a, const vec4& b)
{
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}

inline
f32 calc_length_sq(const vec4& v)
{
	return (v.x * v.x) + (v.y * v.y) + (v.z * v.z) + (v.w * v.w);
}

inline
f32 calc_length(const vec4& v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

inline
void normalize(vec4& v)
{
	// prevent division by zero by checking for zero length and asserting
	f32 length_sq = calc_length_sq(v);
	SM_ASSERT(!is_zero(length_sq));

	// do normalization now that we know length is not zero
	f32 length = sqrtf(length_sq);
	f32 inv_length = 1.0f / length;

	v *= inv_length;
}

inline
vec4 get_normalized(const vec4& v)
{
	vec4 copy = v;
	normalize(copy);
	return copy;
}

inline
vec4 operator*(f32 s, vec4& v)
{
	return v * s;
}

inline
f32 dot(vec4& a, vec4& b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}

inline
f32 distance(vec4& a, vec4& b)
{
	return calc_length(a - b);
}

inline
f32 distance_sq(vec4& a, vec4& b)
{
	return calc_length_sq(a - b);
}

static const vec4 VEC4_ZERO = make_vec4(0.0f, 0.0f, 0.0f, 0.0f);
