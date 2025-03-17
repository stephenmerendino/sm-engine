#pragma once

#include "sm/core/types.h"
#include "sm/core/array.h" 

namespace sm
{
    array_t<byte_t> file_load_bytes(arena_t* arena, const char* filename);
}
