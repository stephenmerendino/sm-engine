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

    };

    static const color_f32_t white_f32	{.r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0};
    static const color_f32_t red_f32	{.r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0};
    static const color_f32_t green_f32	{.r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0};
    static const color_f32_t blue_f32	{.r = 0.0f, .g = 0.0f, .b = 1.0f, .a = 1.0};

    struct color_u8_t
    {
		u8 r = 0;
		u8 g = 0;
		u8 b = 0;
		u8 a = 0;
    };

    static const color_u8_t white_f8	{.r = 1, .g = 1, .b = 1, .a = 1};
    static const color_u8_t red_f8		{.r = 1, .g = 0, .b = 0, .a = 1};
    static const color_u8_t green_f8	{.r = 0, .g = 1, .b = 0, .a = 1};
    static const color_u8_t blue_f8		{.r = 0, .g = 0, .b = 1, .a = 1};
}
