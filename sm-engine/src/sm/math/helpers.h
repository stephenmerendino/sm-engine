#pragma once

#include "sm/core/types.h"

#include <cfloat>
#include <cmath>

namespace sm
{
    constexpr f32 kPi = 3.1415926535897932384626433832795f;
    constexpr f32 k2Pi = 6.283185307179586476925286766559f;

    bool almost_equals(f32 a, f32 b, f32 acceptable_error = FLT_EPSILON);
    bool is_zero(f32 value, f32 acceptable_error = FLT_EPSILON);

    inline f32 deg_to_rad(f32 degrees)
    {
        static const f32 DEGREES_CONVERSION = kPi / 180.0f;
        return degrees * DEGREES_CONVERSION;
    }

    inline f32 rad_to_deg(f32 radians)
    {
        static const f32 RADIANS_CONVERSION = 180.0f / kPi;
        return radians * RADIANS_CONVERSION;
    }

    inline f32 sin_deg(f32 deg)
    {
        return sinf(deg_to_rad(deg));
    }

    inline f32 cos_deg(f32 deg)
    {
        return cosf(deg_to_rad(deg));
    }

    inline f32 tan_deg(f32 deg)
    {
        return tanf(deg_to_rad(deg));
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

    inline f32 remap(f32 value, f32 oldMin, f32 oldMax, f32 newMin, f32 newMax)
    {
        return ((value - oldMin) / (oldMax - oldMin)) * (newMax - newMin) + newMin;
    }
}


