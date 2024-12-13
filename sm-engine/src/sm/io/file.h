#pragma once

#include "sm/core/types.h"
#include "sm/core/array.h" 

namespace sm
{
    static_array_t<byte_t> read_binary_file(arena_t& arena, const char* filename);
}
