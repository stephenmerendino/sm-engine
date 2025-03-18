#pragma once

#include "sm/core/types.h"

namespace sm
{
    struct arena_t
    {
        void*       memory              = nullptr;
        size_t      size_bytes          = 0;
        uintptr_t   head_offset_bytes   = 0;
    };

    arena_t* arena_init(size_t num_bytes);
    arena_t* arena_init(void* arena_memory, size_t num_bytes);
    void     arena_destroy(arena_t* arena);

    void*    arena_alloc(arena_t* arena, size_t num_bytes);
    void*    arena_alloc_zero(arena_t* arena, size_t num_bytes);
    #define  arena_alloc_array(arena, type, num) arena_alloc(arena, sizeof(type) * (num))
    #define  arena_alloc_array_zero(arena, type, num) arena_alloc_zero(arena, sizeof(type) * (num))
    #define  arena_alloc_struct(arena, type) (type*)arena_alloc(arena, sizeof(type))
    #define  arena_alloc_struct_zero(arena, type) (type*)arena_alloc_zero(arena, sizeof(type))

    void     arena_reset(arena_t* arena);
    void     arena_free(arena_t* arena, size_t num_bytes);

#define arena_stack_init(arena_pointer, num_bytes) \
    byte_t stack_bytes[num_bytes]; \
    arena_pointer = arena_init(stack_bytes, num_bytes);
}
