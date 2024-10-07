#pragma once

#include "sm/core/types.h"

namespace sm
{
    struct memory_arena_t
    {
        void*       memory              = nullptr;
        size_t      size_bytes          = 0;
        uintptr_t   head_offset_bytes   = 0;
    };

    memory_arena_t*     arena_init(size_t num_bytes);
    void                arena_destroy(memory_arena_t* arena);
    void                arena_reset(memory_arena_t* arena);

    void*               arena_alloc(memory_arena_t* arena, size_t num_bytes);
    void*               arena_alloc_zero(memory_arena_t* arena, size_t num_bytes);
    #define             arena_alloc_array(arena, type, num) arena_alloc(arena, sizeof(type) * num)
    #define             arena_alloc_array_zero(arena, type, num) arena_alloc_zero(arena, sizeof(type) * num)
    #define             arena_alloc_struct(arena, type) arena_alloc(arena, sizeof(type))
    #define             arena_alloc_struct_zero(arena, type) arena_alloc_zero(arena, sizeof(type))

    void                arena_free(memory_arena_t* arena, size_t num_bytes);
}
