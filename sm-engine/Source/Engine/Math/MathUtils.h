#pragma once

#include "Engine/Core/Types.h"

#include <cfloat>
#include <cmath>

constexpr F32 PI = 3.1415926535897932384626433832795f;
constexpr F32 TWO_PI = 6.283185307179586476925286766559f;

bool AlmostEquals(F32 a, F32 b, F32 maxRelDiff = FLT_EPSILON);
bool IsZero(F32 value);

constexpr F32 DegToRad(F32 degrees)
{
	constexpr F32 DEGREES_CONVERSION = PI / 180.0f;
	return degrees * DEGREES_CONVERSION;
}

constexpr F32 RadToDeg(F32 radians)
{
	constexpr F32 RADIANS_CONVERSION = 180.0f / PI;
	return radians * RADIANS_CONVERSION;
}

inline F32 SinDeg(F32 deg)
{
	return sinf(DegToRad(deg));
}

inline F32 CosDeg(F32 deg)
{
	return cosf(DegToRad(deg));
}

inline F32 TanDeg(F32 deg)
{
	return tanf(DegToRad(deg));
}

template<typename T>
T Clamp(T value, T min, T max)
{
	if (value < min)
		return min;

	if (value > max)
		return max;

	return value;
}

template<typename T>
T Min(T a, T b)
{
	return a < b ? a : b;
}

template<typename T>
T Max(T a, T b)
{
	return a > b ? a : b;
}

inline F32 Remap(F32 value, F32 oldMin, F32 oldMax, F32 newMin, F32 newMax)
{
	return ((value - oldMin) / (oldMax - oldMin)) * (newMax - newMin) + newMin;
}
