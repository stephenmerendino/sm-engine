#pragma once

#include "sm/core/types.h"
#include <cmath>

namespace sm
{
	struct ivec2_t
	{
		i32 x = 0;
		i32 y = 0;

        inline ivec2_t  operator*(i32 scale) const;
        inline ivec2_t  operator/(i32 scale) const;
        inline ivec2_t& operator*=(i32 scale);
        inline ivec2_t& operator/=(i32 scale);
        inline ivec2_t  operator+(const ivec2_t& other) const;
        inline ivec2_t  operator-(const ivec2_t& other) const;
        inline ivec2_t& operator+=(const ivec2_t& other);
        inline ivec2_t& operator-=(const ivec2_t& other);
        inline ivec2_t  operator-() const;
        inline bool     operator==(const ivec2_t& other) const;

        static const ivec2_t ZERO;
	};

    inline ivec2_t ivec2_t::operator*(i32 scale) const
    {
        return ivec2_t(x * scale, y * scale);
    }

    inline ivec2_t ivec2_t::operator/(i32 scale) const
    {
        return ivec2_t(x / scale, y / scale);
    }

    inline ivec2_t& ivec2_t::operator*=(i32 scale)
    {
        x *= scale;
        y *= scale;
        return *this;
    }

    inline ivec2_t& ivec2_t::operator/=(i32 scale)
    {
        x /= scale;
        y /= scale;
        return *this;
    }

    inline ivec2_t ivec2_t::operator+(const ivec2_t& other) const
    {
        return ivec2_t(x + other.x, y + other.y);
    }

    inline ivec2_t ivec2_t::operator-(const ivec2_t& other) const
    {
        return ivec2_t(x - other.x, y - other.y);
    }

    inline ivec2_t& ivec2_t::operator+=(const ivec2_t& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    inline ivec2_t& ivec2_t::operator-=(const ivec2_t& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    inline ivec2_t ivec2_t::operator-() const
    {
        return ivec2_t(-x, -y);
    }

    inline bool ivec2_t::operator==(const ivec2_t& other) const
    {
        return (x == other.x) && (y == other.y);
    }

    inline i32 calc_len_sq(const ivec2_t& iv)
    {
        return (iv.x * iv.x) + (iv.y * iv.y);
    }

    inline f32 calc_len(const ivec2_t& iv)
    {
        return ::sqrtf((f32)calc_len_sq(iv));
    }

    inline i32 dot(const ivec2_t& a, const ivec2_t& b)
    {
        return (a.x * b.x) + (a.y * b.y);
    }

    inline ivec2_t operator*(i32 s, const ivec2_t& iv)
    {
        return ivec2_t(iv.x * s, iv.y * s);
    }
}
