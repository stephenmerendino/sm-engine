#pragma once
#pragma warning(disable:4201)

#include "engine/core/types.h"
#include "engine/math/math_utils.h"

struct ivec2
{
    ivec2(){}
    ivec2(i32 in_x, i32 in_y) :x(in_x), y(in_y) {}

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
ivec2 operator*(const ivec2& iv2, i32 s)
{
    return ivec2(iv2.x * s, iv2.y * s);
}

inline 
ivec2 operator/(const ivec2& iv2, i32 s)
{
    return ivec2(iv2.x / s, iv2.y / s);
}

inline 
ivec2& operator*=(ivec2& iv2, i32 s)
{
    iv2.x *= s;
    iv2.y *= s;
    return iv2;
}

inline 
ivec2& operator/=(ivec2& iv2, i32 s)
{
    iv2.x /= s;
    iv2.y /= s;
    return iv2;
}

inline 
ivec2 operator+(const ivec2& a, const ivec2& b)
{
    return ivec2(a.x + b.x, a.y + b.y);
}

inline 
ivec2& operator+=(ivec2& iv2, const ivec2& add)
{
   iv2.x += add.x; 
   iv2.y += add.y; 
   return iv2;
}

inline 
ivec2 operator-(const ivec2& a, const ivec2& b)
{
    return ivec2(a.x - b.x, a.y - b.y); 
}

inline 
ivec2& operator-=(ivec2& iv2, const ivec2& sub)
{
    iv2.x -= sub.x;
    iv2.y -= sub.y;
    return iv2;
}

inline 
ivec2 operator-(const ivec2& iv2)
{
    return ivec2(-iv2.x, -iv2.y);
}

inline 
bool operator==(const ivec2& a, const ivec2& b)
{
    return (a.x == b.x) && (a.y == b.y);
}

inline 
i32 calc_length_sq(const ivec2& iv2)
{
    return (iv2.x * iv2.x) + (iv2.y * iv2.y);
}

inline 
f32 calc_length(const ivec2& iv2)
{
    return sqrt(calc_length_sq(iv2)); 
}

inline 
ivec2 operator*(i32 s, const ivec2& iv2)
{
    return ivec2(iv2.x * s, iv2.y * s);
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
