#pragma once

#include "engine/core/types.h"
#include "engine/platform/windows_include.h"

typedef volatile i32 atomic_int_t;

__forceinline
i32 atomic_incr(atomic_int_t* value)
{
	return (i32)::InterlockedIncrementNoFence((volatile LONG*)value);
}

__forceinline
i32 atomic_decr(atomic_int_t* value)
{
	return (i32)::InterlockedDecrementNoFence((volatile LONG*)value);
}

__forceinline
i32 atomic_add(atomic_int_t* addend, i32 value)
{
	return (i32)::InterlockedAddNoFence((volatile LONG*)addend, (LONG)value);
}

__forceinline
i32 atomic_set(atomic_int_t* target, i32 value)
{
	return (i32)::InterlockedExchange((volatile LONG*)target, (LONG)value);
}
