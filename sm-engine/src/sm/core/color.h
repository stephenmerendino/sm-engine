#pragma once

#include  "sm/core/random.h"
#include "sm/core/types.h"
#include "sm/math/vec3.h"
#include "sm/math/vec4.h"

namespace sm
{
    struct color_f32_t
    {
		f32 r = 0.0f;
		f32 g = 0.0f;
		f32 b = 0.0f;
		f32 a = 0.0f;

        static const color_f32_t WHITE;
        static const color_f32_t RED;
        static const color_f32_t GREEN;
        static const color_f32_t BLUE;
        static const color_f32_t YELLOW;
    };

    struct color_u8_t
    {
		u8 r = 0;
		u8 g = 0;
		u8 b = 0;
		u8 a = 0;

        static const color_u8_t WHITE;
        static const color_u8_t RED;
        static const color_u8_t GREEN;
        static const color_u8_t BLUE;
        static const color_u8_t YELLOW;
    };

    inline vec3_t to_vec3(const color_f32_t& color)
    {
        return { color.r, color.g, color.b };
    }

    inline vec4_t to_vec4(const color_f32_t& color)
    {
        return { color.r, color.g, color.b, color.a };
    }

    inline color_f32_t color_gen_random()
    {
        return { .r = random_gen_zero_to_one(), .g = random_gen_zero_to_one(), .b = random_gen_zero_to_one(), .a = 1.0f };
    }
}
