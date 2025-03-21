#pragma once
#pragma warning(disable:4201)

#include "sm/core/types.h"
#include "sm/core/assert.h"
#include "sm/math/helpers.h"
#include <cmath>

namespace sm
{
    struct vec2_t
    {
        f32 x = 0.0f;
        f32 y = 0.0f;

    	inline vec2_t   operator*(f32 s) const;
    	inline vec2_t   operator/(f32 s) const;
    	inline vec2_t&  operator*=(f32 s);
    	inline vec2_t&  operator/=(f32 s);
    	inline vec2_t   operator+(const vec2_t& other) const;
    	inline vec2_t   operator-(const vec2_t& other) const;
    	inline vec2_t&  operator+=(const vec2_t& other);
    	inline vec2_t&  operator-=(const vec2_t& other);
    	inline vec2_t   operator-() const;
    	inline bool     operator==(const vec2_t& other) const;

        static const vec2_t ZERO;
    };

    inline vec2_t vec2_t::operator*(f32 s) const
    {
        return vec2_t(x * s, y * s );
    }
    
    inline vec2_t vec2_t::operator/(f32 s) const
    {
        return vec2_t( x / s, y / s );
    }
    
    inline vec2_t& vec2_t::operator*=(f32 s)
    {
    	x *= s;
    	y *= s;
    	return *this;
    }
    
    inline vec2_t& vec2_t::operator/=(f32 s)
    {
    	x /= s;
    	y /= s;
    	return *this;
    }
    
    inline vec2_t vec2_t::operator+(const vec2_t& other) const
    {
    	return vec2_t(x + other.x, y + other.y);
    }
    
    inline vec2_t vec2_t::operator-(const vec2_t& other) const
    {
    	return vec2_t(x - other.x, y - other.y);
    }
    
    inline vec2_t& vec2_t::operator+=(const vec2_t& other)
    {
    	x += other.x;
    	y += other.y;
    	return *this;
    }
    
    inline vec2_t& vec2_t::operator-=(const vec2_t& other)
    {
    	x -= other.x;
    	y -= other.y;
    	return *this;
    }
    
    inline vec2_t vec2_t::operator-() const
    {
    	return vec2_t(-x, -y);
    }
    
    inline bool vec2_t::operator==(const vec2_t& other) const
    {
    	return (x == other.x) && (y == other.y);
    }

    inline f32 calc_len_sq(const vec2_t& v)
    {
    	return (v.x * v.x) + (v.y * v.y);
    }
    
    inline f32 calc_len(const vec2_t& v)
    {
        return ::sqrtf(calc_len_sq(v));
    }

    inline void normalize(vec2_t& v)
    {
        // prevent division by zero by checking for zero length and asserting
        f32 len_sq = calc_len_sq(v);
        SM_ASSERT(!is_zero(len_sq));

        // do normalization now that we know length is not zero
        f32 len = ::sqrtf(len_sq);
        f32 inv_len = 1.0f / len;

        v *= inv_len;
    }

    inline vec2_t normalized(const vec2_t& v)
    {
    	vec2_t copy = v;
        normalize(copy);
    	return copy;
    }
    
    inline vec2_t operator*(f32 s, const vec2_t& v)
    {
        return v * s;
    }

    inline f32 dot(const vec2_t& a, const vec2_t& b)
    {
    	return (a.x * b.x) + (a.y * b.y);
    }
    
    inline vec2_t polar_to_cartesian_rads(f32 radians, f32 radius = 1.0f)
    {
        return radius * vec2_t(cosf(radians), sinf(radians));
    }

    inline vec2_t polar_to_cartesian_degs(f32 deg, f32 radius = 1.0f)
    {
        return radius * vec2_t(cos_deg(deg), sin_deg(deg));
    }
}
