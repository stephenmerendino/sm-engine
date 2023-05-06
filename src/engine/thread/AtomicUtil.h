#pragma once

#include "engine/core/Types.h"
#include "engine/platform/WindowsInclude.h"

__forceinline
I32 AtomicIncr(volatile I32* value)
{
	return (I32)::InterlockedIncrementNoFence((volatile LONG*)value);
}

__forceinline
I32 AtomicDecr(volatile I32* value)
{
	return (I32)::InterlockedDecrementNoFence((volatile LONG*)value);
}

__forceinline
I32 AtomicAdd(volatile I32* addend, I32 value)
{
	return (I32)::InterlockedAddNoFence((volatile LONG*)addend, (LONG)value);
}

__forceinline
I32 AtomicSet(volatile I32* target, I32 value)
{
	return (I32)::InterlockedExchange((volatile LONG*)target, (LONG)value);
}
