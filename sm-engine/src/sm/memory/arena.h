#pragma once

#include "sm/core/types.h"

namespace sm::memory
{
    struct arena_t
    {
        void*       memory              = nullptr;
        size_t      size_bytes          = 0;
        uintptr_t   head_offset_bytes   = 0;
    };

    arena_t* init_arena(size_t num_bytes);
    void     destroy_arena(arena_t* arena);
    void     reset_arena(arena_t* arena);

    void*    alloc(arena_t* arena, size_t num_bytes);
    void*    alloc_zero(arena_t* arena, size_t num_bytes);
    #define  alloc_array(arena, type, num) alloc(arena, sizeof(type) * (num))
    #define  alloc_array_zero(arena, type, num) alloc_zero(arena, sizeof(type) * (num))
    #define  alloc_struct(arena, type) (type*)alloc(arena, sizeof(type))
    #define  alloc_struct_zero(arena, type) (type*)alloc_zero(arena, sizeof(type))

    void     free(arena_t* arena, size_t num_bytes);
}
