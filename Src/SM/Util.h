#pragma once

#include "SM/Memory.h"

namespace SM
{
    #define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))
    #define UNUSED(x) (void*)&x

    template <typename T>
        inline void Swap(T& a, T& b)
        {
            T copy = a;
            a = b;
            b = copy;
        }

    char* ConcatenateStrings(const char* s1, const char* s2, LinearAllocator* allocator = GetCurrentAllocator());
}
