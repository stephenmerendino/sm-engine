#pragma once
#pragma warning(disable:4201)

#include "engine/math/math_utils.h"
#include "engine/core/types.h"
#include "engine/core/assert.h"

#include <cmath>

struct vec2
{
	inline vec2() {}

	union
	{
		struct
		{
			f32 x;
			f32 y;
		};

		f32 data[2];
	};
};

inline
vec2 make_vec2(f32 in_x, f32 in_y)
{
	vec2 v;
	v.x = in_x;
	v.y = in_y;
	return v;
}

inline
vec2 operator*(const vec2& v, f32 s)
{
	return make_vec2(v.x * s, v.y * s);
}

inline
vec2 operator/(const vec2& v, f32 s)
{
	return make_vec2(v.x * s, v.y * s);
}

inline
vec2& operator*=(vec2& v, f32 s)
{
	v.x *= s;
	v.y *= s;
	return v;
}

inline
vec2& operator/=(vec2& v, f32 s)
{
	v.x *= s;
	v.y *= s;
	return v;
}

inline
vec2 operator+(const vec2& a, const vec2& b)
{
	return make_vec2(a.x + b.x, a.y + b.y);
}

inline
vec2& operator+=(vec2& v, const vec2& add)
{
	v.x += add.x;
	v.y += add.y;
	return v;
}

inline
vec2 operator-(const vec2& a, const vec2& b)
{
	return make_vec2(a.x - b.x, a.y - b.y);
}

inline
vec2& operator-=(vec2& v, const vec2& sub)
{
	v.x -= sub.x;
	v.y -= sub.y;
	return v;
}

inline
vec2 operator-(const vec2& v)
{
	return make_vec2(-v.x, -v.y);
}


inline
bool operator==(const vec2& a, const vec2& b)
{
	return (a.x == b.x) && (a.y == b.y);
}

inline
f32 calc_length_sq(const vec2& v)
{
	return (v.x * v.x) + (v.y * v.y);
}

inline
f32 calc_length(const vec2& v)
{
	return sqrtf(calc_length_sq(v));
}

inline
void normalize(vec2& v)
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
vec2 get_normalized(const vec2& v)
{
	vec2 copy = v;
	normalize(copy);
	return copy;
}

inline
vec2 operator*(f32 s, const vec2& v)
{
	return v * s;
}

inline
f32 dot(const vec2& a, const vec2& b)
{
	return (a.x * b.x) + (a.y * b.y);
}

inline
f32 distance(const vec2& a, const vec2& b)
{
	return calc_length(a - b);
}

inline
f32 distance_sq(const vec2& a, const vec2& b)
{
	return calc_length_sq(a - b);
}

inline
vec2 calc_2d_polar_to_xy_rad(f32 radians, f32 radius = 1.0f)
{
	return radius * make_vec2(cosf(radians), sinf(radians));
}

inline
vec2 calc_2d_polar_to_xy_deg(f32 deg, f32 radius = 1.0f)
{
	return radius * make_vec2(cos_deg(deg), sin_deg(deg));
}

static const vec2 VEC2_ZERO = make_vec2(0, 0);
