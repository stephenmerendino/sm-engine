#include "sm/memory/arena.h"

#include "sm/core/assert.h"

#include <cstdlib>
#include <cstring>

using namespace sm;

sm::arena_t* sm::arena_init(size_t num_bytes)
{
    arena_t* arena = new arena_t;
    arena->memory = ::malloc(num_bytes);
    SM_ASSERT(arena->memory);
    arena->size_bytes = num_bytes;
    arena->head_offset_bytes = 0;
    return arena;
}

arena_t* sm::arena_init(void* arena_memory, size_t num_bytes)
{
    SM_ASSERT(arena_memory);
    arena_t* arena = new arena_t;
    arena->memory = arena_memory;
    arena->size_bytes = num_bytes;
    arena->head_offset_bytes = 0;
    return arena;
}

void sm::arena_destroy(arena_t* arena)
{
    SM_ASSERT(arena);
    SM_ASSERT(arena->memory);
    ::free(arena->memory);
    delete arena;
}

void* sm::arena_alloc(arena_t* arena, size_t num_bytes)
{
    SM_ASSERT(arena->head_offset_bytes + num_bytes <= arena->size_bytes);
    void* allocated_mem = (void*)( (uintptr_t)arena->memory + (uintptr_t)arena->head_offset_bytes );
    arena->head_offset_bytes += num_bytes;
    return allocated_mem;
}

void* sm::arena_alloc_zero(arena_t* arena, size_t num_bytes)
{
    SM_ASSERT(arena->head_offset_bytes + num_bytes <= arena->size_bytes);
    void* allocated_mem = (void*)( (uintptr_t)arena->memory + (uintptr_t)arena->head_offset_bytes );
    memset(allocated_mem, 0, num_bytes);
    arena->head_offset_bytes += num_bytes;
    return allocated_mem;
}

void sm::arena_reset(arena_t* arena)
{
    arena->head_offset_bytes = 0;
}

void sm::arena_free(arena_t* arena, size_t num_bytes)
{
    SM_ASSERT(arena->head_offset_bytes >= num_bytes);
    arena->head_offset_bytes -= num_bytes;
}
