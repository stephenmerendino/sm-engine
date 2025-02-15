#pragma once

#include "sm/core/types.h"

namespace sm
{
    void init_random();
    f32 gen_random_zero_to_one();
    f32 gen_random_between(f32 low, f32 high);
    i32 gen_random_between(i32 low, i32 high);
}
