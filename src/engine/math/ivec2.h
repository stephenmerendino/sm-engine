#pragma once
#pragma warning(disable:4201)

#include "engine/core/types.h"
#include "engine/math/math_utils.h"

struct ivec2
{
    inline ivec2(){}
    inline ivec2(i32 in_x, i32 in_y) 
        :x(in_x)
        ,y(in_y)
    {}

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

static const ivec2 IVEC2_ZERO(0, 0);

inline
ivec2 operator*(const ivec2& iv, i32 s)
{
    return ivec2(iv.x * s, iv.y * s);
}

inline 
ivec2 operator/(const ivec2& iv, i32 s)
{
    return ivec2(iv.x / s, iv.y / s);
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
    return ivec2(a.x + b.x, a.y + b.y);
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
    return ivec2(a.x - b.x, a.y - b.y); 
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
    return ivec2(-iv.x, -iv.y);
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
    return ivec2(iv.x * s, iv.y * s);
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
