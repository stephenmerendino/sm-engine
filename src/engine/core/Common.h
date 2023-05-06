#pragma once

#include "engine/core/Assert.h"
#include "engine/core/Types.h"
#include "engine/core/Debug.h"

#define Unused(var) var
#define MemZero(x) memset(&x, 0, sizeof(x));

template<typename T>
void Swap(T& a, T& b)
{
	T temp = a;
	a = b;
	b = temp;
}
