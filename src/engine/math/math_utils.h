#pragma once

#include "engine/core/types.h"

#include <cfloat>
#include <cmath>

constexpr f32 PI = 3.1415926535897932384626433832795f;
constexpr f32 TWO_PI = 6.283185307179586476925286766559f;

bool almost_equals(f32 a, f32 b, f32 maxRelDiff = FLT_EPSILON);
bool is_zero(f32 s);

constexpr f32 deg_to_rad(f32 degrees) 
{ 
	constexpr f32 DEGREES_CONVERSION = PI / 180.0f;
	return degrees * DEGREES_CONVERSION; 
}

constexpr f32 rad_to_deg(f32 radians) 
{ 
	constexpr f32 RADIANS_CONVERSION = 180.0f / PI;
	return radians * RADIANS_CONVERSION; 
}

inline
f32 sin_deg(f32 degs)
{
	return sinf(deg_to_rad(degs));
}

inline
f32 cos_deg(f32 degs)
{
	return cosf(deg_to_rad(degs));
}

inline
f32 tan_deg(f32 degs)
{
	return tanf(deg_to_rad(degs));
}

template<typename T>
T clamp(T value, T min, T max)
{
	if (value < min)
		return min;

	if (value > max)
		return max;

	return value;
}

template<typename T>
T min(T a, T b)
{
	return a < b ? a : b;
}

template<typename T>
T max(T a, T b)
{
	return a > b ? a : b;
}

inline
f32 remap(f32 value, f32 old_min, f32 old_max, f32 new_min, f32 new_max)
{
	return ((value - old_min) / (old_max - old_min)) * (new_max - new_min) + new_min;
}
