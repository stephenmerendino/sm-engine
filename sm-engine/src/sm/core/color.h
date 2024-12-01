#pragma once

#include "sm/core/types.h"

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
    };
}
