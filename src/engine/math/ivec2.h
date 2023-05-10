#pragma once
#pragma warning(disable:4201)

#include "engine/core/types.h"
#include "engine/math/math_utils.h"

struct ivec2
{
    inline ivec2(){}

	union
	{
		struct
		{
			i32 x = 0;
			i32 y = 0;
		};

		i32 data[2];
	};
};

inline
ivec2 make_ivec2(i32 x, i32 y)
{
    ivec2 iv2;
    iv2.x = x;
    iv2.y = y;
    return iv2;
}

inline
ivec2 operator*(const ivec2& iv, i32 s)
{
    return make_ivec2(iv.x * s, iv.y * s);
}

inline 
ivec2 operator/(const ivec2& iv, i32 s)
{
    return make_ivec2(iv.x / s, iv.y / s);
}

inline 
ivec2& operator*=(ivec2& iv, i32 s)
{
    iv.x *= s;
    iv.y *= s;
    return iv;
}

inline 
ivec2& operator/=(ivec2& iv, i32 s)
{
    iv.x /= s;
    iv.y /= s;
    return iv;
}

inline 
ivec2 operator+(const ivec2& a, const ivec2& b)
{
    return make_ivec2(a.x + b.x, a.y + b.y);
}

inline 
ivec2& operator+=(ivec2& iv, const ivec2& add)
{
   iv.x += add.x; 
   iv.y += add.y; 
   return iv;
}

inline 
ivec2 operator-(const ivec2& a, const ivec2& b)
{
    return make_ivec2(a.x - b.x, a.y - b.y); 
}

inline 
ivec2& operator-=(ivec2& iv, const ivec2& sub)
{
    iv.x -= sub.x;
    iv.y -= sub.y;
    return iv;
}

inline 
ivec2 operator-(const ivec2& iv)
{
    return make_ivec2(-iv.x, -iv.y);
}

inline 
bool operator==(const ivec2& a, const ivec2& b)
{
    return (a.x == b.x) && (a.y == b.y);
}

inline 
i32 calc_length_sq(const ivec2& iv)
{
    return (iv.x * iv.x) + (iv.y * iv.y);
}

inline 
f32 calc_length(const ivec2& iv)
{
    return sqrt(calc_length_sq(iv)); 
}

inline 
ivec2 operator*(i32 s, const ivec2& iv)
{
    return make_ivec2(iv.x * s, iv.y * s);
}

inline 
i32 dot(const ivec2& a, const ivec2& b)
{
    return (a.x * b.x) + (a.y * b.y);
}

inline f32 dist(const ivec2& a, const ivec2& b)
{
    return calc_length(a - b); 
}

inline i32 dist_sq(const ivec2& a, const ivec2& b)
{
    return calc_length_sq(a - b); 
}

static const ivec2 IVEC2_ZERO = make_ivec2(0, 0);
