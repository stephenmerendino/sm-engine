#pragma once

#include "engine/core/types.h"
#include "engine/math/vec4.h"

enum class Color : u8
{
    kWhite,
    kRed,
    kGreen,
    kBlue
};

inline
vec4 color_get(Color c)
{
    switch(c)
    {
        case Color::kWhite: return make_vec4(1.0f, 1.0f, 1.0f, 1.0f);
        case Color::kRed: return make_vec4(1.0f, 0.0f, 0.0f, 1.0f);
        case Color::kGreen: return make_vec4(0.0f, 1.0f, 0.0f, 1.0f);
        case Color::kBlue: return make_vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    return make_vec4(0.0f, 0.0f, 0.0f, 0.0f);
}
