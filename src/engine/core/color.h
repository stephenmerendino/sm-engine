#pragma once

#include "engine/core/types.h"
#include "engine/math/vec4.h"

enum class Color : u8
{
    kWhite
};

inline
vec4 get_color(Color c)
{
    switch(c)
    {
        case Color::kWhite: return make_vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    return make_vec4(0.0f, 0.0f, 0.0f, 0.0f);
}
