#pragma once

#include "sm/core/types.h"
#include "sm/core/string.h"

namespace sm
{
    typedef u64 hash_t;

    hash_t hash(const string_t& str);
    hash_t hash(const char* str);
}
