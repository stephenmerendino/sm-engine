#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Platform/WindowsInclude.h"

typedef volatile I32 AtomicInt;

__forceinline I32 AtomicIncr(AtomicInt* value)
{
	return (I32)::InterlockedIncrementNoFence((volatile LONG*)value);
}

__forceinline I32 AtomicDecr(AtomicInt* value)
{
	return (I32)::InterlockedDecrementNoFence((volatile LONG*)value);
}

__forceinline I32 AtomicAdd(AtomicInt* addend, I32 value)
{
	return (I32)::InterlockedAddNoFence((volatile LONG*)addend, (LONG)value);
}

__forceinline I32 AtomicSet(AtomicInt* target, I32 value)
{
	return (I32)::InterlockedExchange((volatile LONG*)target, (LONG)value);
}
#pragma once
