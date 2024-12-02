#pragma once

#include "sm/core/assert.h"
#include "sm/core/types.h"
#include "sm/math/helpers.h"
#include <cmath>

namespace sm
{
	struct vec4_t
	{
		f32 x = 0.0f;
		f32 y = 0.0f;
		f32 z = 0.0f;
		f32 w = 0.0f;

        inline vec4_t   operator*(f32 s) const;
        inline vec4_t   operator/(f32 s) const;
        inline vec4_t&  operator*=(f32 s);
        inline vec4_t&  operator/=(f32 s);
        inline vec4_t   operator+(const vec4_t& other) const;
        inline vec4_t   operator-(const vec4_t& other) const;
        inline vec4_t&  operator+=(const vec4_t& other);
        inline vec4_t&  operator-=(const vec4_t& other);
        inline vec4_t   operator-() const;
        inline bool     operator==(const vec4_t& other) const;
	};

    inline vec4_t vec4_t::operator*(f32 s) const
    {
        return { x * s, y * s, z * s, w * s };
    }

    inline vec4_t vec4_t::operator/(f32 s) const
    {
        f32 inv_s = 1.0f / s;
        return { x * inv_s, y * inv_s, z * inv_s, w * inv_s };
    }

    inline vec4_t& vec4_t::operator*=(f32 s)
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }

    inline vec4_t& vec4_t::operator/=(f32 s)
    {
        f32 inv_s = 1.0f / s;
        x *= inv_s;
        y *= inv_s;
        z *= inv_s;
        w *= inv_s;
        return *this;
    }

    inline vec4_t vec4_t::operator+(const vec4_t& other) const
    {
        return { x + other.x, y + other.y, z + other.z, w + other.w };
    }

    inline vec4_t vec4_t::operator-(const vec4_t& other) const
    {
        return { x - other.x, y - other.y, z - other.z, w - other.w };
    }

    inline vec4_t& vec4_t::operator+=(const vec4_t& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    inline vec4_t& vec4_t::operator-=(const vec4_t& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    inline vec4_t vec4_t::operator-() const
    {
        return { -x, -y, -z, -w };
    }

    inline bool vec4_t::operator==(const vec4_t& other) const
    {
        return (x == other.x) && (y == other.y) && (z == other.z) && (w == other.w);
    }

    inline f32 calc_len_sq(const vec4_t& v)
    {
        return (v.x * v.x) + (v.y * v.y) + (v.z * v.z) + (v.w * v.w);
    }

    inline f32 calc_len(const vec4_t& v)
    {
        return ::sqrtf(calc_len_sq(v));
    }

    inline void normalize(vec4_t& v)
    {
        // prevent division by zero by checking for zero length and asserting
        f32 len_sq = calc_len_sq(v);
        SM_ASSERT(!is_zero(len_sq));

        // do normalization now that we know length is not zero
        f32 length = sqrtf(len_sq);
        f32 invLength = 1.0f / length;

        v *= invLength;
    }

    inline vec4_t normalized(const vec4_t& v)
    {
        vec4_t copy = v;
        normalize(copy);
        return copy;
    }

    inline vec4_t operator*(f32 s, const vec4_t& v)
    {
        return v * s;
    }

    inline f32 dot(const vec4_t& a, const vec4_t& b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
    }
}
