#pragma once

#include "engine/core/types.h"
#include "engine/platform/windows_include.h"

__forceinline
i32 atomic_incr(volatile i32* value)
{
	return (i32)::InterlockedIncrementNoFence((volatile LONG*)value);
}

__forceinline
i32 atomic_decr(volatile i32* value)
{
	return (i32)::InterlockedDecrementNoFence((volatile LONG*)value);
}

__forceinline
i32 atomic_add(volatile i32* addend, i32 value)
{
	return (i32)::InterlockedAddNoFence((volatile LONG*)addend, (LONG)value);
}

__forceinline
i32 atomic_set(volatile i32* target, i32 value)
{
	return (i32)::InterlockedExchange((volatile LONG*)target, (LONG)value);
}
