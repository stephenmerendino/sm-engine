#pragma once

#include "sm/core/types.h"

namespace sm
{
    void random_init();
    f32 random_gen_zero_to_one();
    f32 random_gen_between(f32 low, f32 high);
    i32 random_gen_between(i32 low, i32 high);
}
