#pragma once

#include "sm/memory/arena.h"
#include "sm/core/assert.h"

#include <cstring>

namespace sm
{
    // static_array
    template<typename T>
    struct static_array
    {
        T*      data  = nullptr;
        size_t  size  = 0;

        T& operator[](size_t index)
        {
            SM_ASSERT(index <= size);
            return data[index];
        }

        const T& operator[](size_t index) const
        {
            SM_ASSERT(index <= size);
            return data[index];
        }
    };

    template<typename T>
    static_array<T> static_array_init(memory_arena_t* arena, size_t size)
    {
        static_array<T> arr;
        arr.data = (T*)arena_alloc_array_zero(arena, T, size);
        arr.size = size;
        return arr;
    }

    template<typename T>
    void static_array_copy(static_array<T> dst_array, const T* src_data, size_t num_items)
    {
        SM_ASSERT(num_items <= dst_array.size);
        memcpy(dst_array.data, src_data, num_items * sizeof(T));
    }

    template<typename T>
    void static_array_copy(static_array<T> dst_array, static_array<T> src_array, size_t num_items)
    {
        SM_ASSERT(num_items <= src_array.size);
        SM_ASSERT(num_items <= dst_array.size);
        memcpy(dst_array.data, src_array.data, num_items * sizeof(T));
    }
}
