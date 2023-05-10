#pragma once
#pragma warning(disable:4201)

#include <engine/math/Vec2.h>

struct vec3
{
    inline vec3(){}
    inline vec3(f32 in_x, f32 in_y, f32 in_z)
        :x(in_x)
        ,y(in_y)
        ,z(in_z)
    {}

    union
    {
        struct
        {
            f32 x;
            f32 y;
            f32 z;
        };

        f32 data[3];
    };
};

static const vec3 VEC3_ZERO      = vec3(0.0f, 0.0f, 0.0f);
static const vec3 VEC3_FORWARD   = vec3(1.0f, 0.0f, 0.0f);
static const vec3 VEC3_BACKWARD  = vec3(-1.0f, 0.0f, 0.0f);
static const vec3 VEC3_UP        = vec3(0.0f, 0.0f, 1.0f);
static const vec3 VEC3_DOWN      = vec3(0.0f, 0.0f, -1.0f);
static const vec3 VEC3_LEFT      = vec3(0.0f, 1.0f, 0.0f);
static const vec3 VEC3_RIGHT     = vec3(0.0f, -1.0f, 0.0f);

inline 
vec3 operator*(const vec3& v,f32 s)
{
	return vec3(v.x * s, v.y * s, v.z * s);
}

inline 
vec3 operator/(const vec3& v, f32 s)
{
	f32 inv_s = 1.0f / s;
	return vec3(v.x * inv_s, v.y * inv_s, v.z * inv_s);
}

inline 
vec3& operator*=(vec3& v, f32 s)
{
	v.x *= s;
	v.y *= s;
	v.z *= s;
	return v;
}

inline 
vec3& operator/=(vec3& v, f32 s)
{
	f32 inv_s = 1.0f / s;
	v.x *= inv_s;
	v.y *= inv_s;
	v.z *= inv_s;
	return v;
}

inline 
vec3 operator+(const vec3& a, const vec3& b)
{
	return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline 
vec3& operator+=(vec3& v, const vec3& add)
{
	v.x += add.x;
	v.y += add.y;
	v.z += add.z;
	return v;
}

inline 
vec3 operator-(const vec3& a, const vec3& b)
{
	return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline 
vec3& operator-=(vec3& v, const vec3& sub)
{
	v.x -= sub.x;
	v.y -= sub.y;
	v.z -= sub.z;
	return v;
}

inline 
vec3 operator-(const vec3& v)
{
	return vec3(-v.x, -v.y, -v.z);
}

inline 
bool operator==(const vec3& a, const vec3& b)
{
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

inline 
f32 calc_length_sq(const vec3& v)
{
	return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
}

inline 
f32 calc_length(const vec3& v)
{
	return sqrtf(calc_length_sq(v));
}

inline 
void normalize(vec3& v)
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
vec3 get_normalized(const vec3& v)
{
    vec3 copy = v;
    normalize(copy);
    return copy;
}

inline 
vec3 operator*(f32 s, const vec3& v)
{
	return v * s;
}

inline
f32 dot(const vec3& a, const vec3& b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

inline 
vec3 cross(const vec3& a, const vec3& b)
{
	vec3 cross;
	cross.x = a.y * b.z - a.z * b.y;
	cross.y = a.z * b.x - a.x * b.z;
	cross.z = a.x * b.y - a.y * b.x;
	return cross;
}

inline 
f32 distance(const vec3& a, const vec3& b)
{
    return calc_length(a - b);
}

inline 
f32 DistanceSquared(const vec3& a, const vec3& b)
{
    return calc_length_sq(a - b);
}
