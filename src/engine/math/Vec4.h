#pragma once
#pragma warning(disable:4201)

#include "engine/core/assert.h"
#include "engine/core/types.h"
#include "engine/math/math_utils.h"
#include "engine/math/vec3.h"

struct vec4
{
    inline vec4(){}
    inline vec4(f32 in_x, f32 in_y, f32 in_z, f32 in_w)
        :x(in_x)
        ,y(in_y)
        ,z(in_z)
        ,w(in_w)
    {}

    union
    {
        struct
        {
            f32 x;
            f32 y;
            f32 z;
            f32 w;
        };

        struct
        {
            vec3 xyz;
            f32 pad0;
        };

        f32 data[4];
    };
};

static const vec4 VEC4_ZERO = vec4(0.0f, 0.0f, 0.0f, 0.0f);

inline 
vec4 operator*(const vec4& v, f32 s) 
{
	return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}

inline 
vec4 operator/(const vec4& v, f32 s) 
{
	f32 inv_s = 1.0f / s;
	return vec4(v.x * inv_s, v.y * inv_s, v.z * inv_s, v.w * inv_s);
}

inline 
vec4& operator*=(vec4& v, f32 s)
{
	v.x *= s;
	v.y *= s;
	v.z *= s;
	v.w *= s;
	return v;
}

inline 
vec4& operator/=(vec4& v, f32 s)
{
	f32 inv_s = 1.0f / s;
	v.x *= inv_s;
	v.y *= inv_s;
	v.z *= inv_s;
	v.w *= inv_s;
	return v;
}

inline 
vec4 operator+(const vec4& a, const vec4& b) 
{
	return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

inline 
vec4& operator+=(vec4& v, const vec4& add)
{
	v.x += add.x;
	v.y += add.y;
	v.z += add.z;
	v.w += add.w;
	return v;
}

inline 
vec4 operator-(const vec4& a, const vec4& b) 
{
	return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

inline 
vec4& operator-=(vec4& v, const vec4& sub)
{
	v.x -= sub.x;
	v.y -= sub.y;
	v.z -= sub.z;
	v.w -= sub.w;
	return v;
}

inline 
vec4 operator-(const vec4& v) 
{
	return vec4(-v.x, -v.y, -v.z, -v.w);
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
	ASSERT(!is_zero(length_sq));

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
f32 dot(vec4& a,  vec4& b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}

inline 
f32 distance(vec4& a,  vec4& b)
{
    return calc_length(a - b);
}

inline 
f32 distance_sq( vec4& a,  vec4& b)
{
    return calc_length_sq(a - b);
}
