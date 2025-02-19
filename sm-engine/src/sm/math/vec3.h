#pragma once

#include "sm/core/assert.h"
#include "sm/core/types.h"
#include "sm/math/helpers.h"
#include <cmath>

namespace sm
{
	struct vec3_t
	{
        f32 x = 0.0f;
        f32 y = 0.0f;
        f32 z = 0.0f;

        inline vec3_t operator*(f32 s) const;
        inline vec3_t operator/(f32 s) const;
        inline vec3_t& operator*=(f32 s);
        inline vec3_t& operator/=(f32 s);
        inline vec3_t operator+(const vec3_t& other) const;
        inline vec3_t& operator+=(const vec3_t& other);
        inline vec3_t operator-(const vec3_t& other) const;
        inline vec3_t& operator-=(const vec3_t& other);
        inline vec3_t operator-() const;
        inline bool operator==(const vec3_t& other);

        static const vec3_t ZERO;
        static const vec3_t X_AXIS;
        static const vec3_t Y_AXIS;
        static const vec3_t Z_AXIS;
        static const vec3_t WORLD_FORWARD;
        static const vec3_t WORLD_BACKWARD;
        static const vec3_t WORLD_UP;
        static const vec3_t WORLD_DOWN;
        static const vec3_t WORLD_LEFT;
        static const vec3_t WORLD_RIGHT;
	};

    inline vec3_t vec3_t::operator*(f32 s) const
    {
        return vec3_t(x * s, y * s, z * s);
    }

    inline vec3_t vec3_t::operator/(f32 s) const
    {
        f32 inv_s = 1.0f / s;
        return vec3_t(x * inv_s, y * inv_s, z * inv_s);
    }

    inline vec3_t& vec3_t::operator*=(f32 s)
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    inline vec3_t& vec3_t::operator/=(f32 s)
    {
        f32 inv_s = 1.0f / s;
        x *= inv_s;
        y *= inv_s;
        z *= inv_s;
        return *this;
    }

    inline vec3_t vec3_t::operator+(const vec3_t& other) const
    {
        return vec3_t(x + other.x, y + other.y, z + other.z);
    }

    inline vec3_t& vec3_t::operator+=(const vec3_t& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    inline vec3_t vec3_t::operator-(const vec3_t& other) const
    {
        return vec3_t(x - other.x, y - other.y, z - other.z);
    }

    inline vec3_t& vec3_t::operator-=(const vec3_t& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    inline vec3_t vec3_t::operator-() const
    {
        return vec3_t(-x, -y, -z);
    }

    inline bool vec3_t::operator==(const vec3_t& other)
    {
        return (x == other.x) && (y == other.y) && (z == other.z);
    }

    inline f32 calc_len_sq(const vec3_t& v)
    {
    	return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
    }

    inline f32 calc_len(const vec3_t& v)
    {
        return ::sqrtf(calc_len_sq(v));
    }

    inline void normalize(vec3_t& v)
    {
        // prevent division by zero by checking for zero length and asserting
        f32 len_sq = calc_len_sq(v);
        SM_ASSERT(!is_zero(len_sq));

        // do normalization now that we know length is not zero
        f32 len = ::sqrtf(len_sq);
        f32 inv_len = 1.0f / len;

        v *= inv_len;
    }

    inline vec3_t normalized(const vec3_t& v)
    {
    	vec3_t copy = v;
        normalize(copy);
    	return copy;
    }

    inline vec3_t operator*(f32 s, const vec3_t& v)
    {
        return v * s;
    }

    inline f32 dot(const vec3_t& a, const vec3_t& b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    }

    inline vec3_t cross(const vec3_t& a, const vec3_t& b)
    {
        vec3_t cross;
        cross.x = a.y * b.z - a.z * b.y;
        cross.y = a.z * b.x - a.x * b.z;
        cross.z = a.x * b.y - a.y * b.x;
        return cross;
    }
}
