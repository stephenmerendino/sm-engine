#pragma once

#include "Engine/Core/Assert.h"
#include "Engine/Core/Types.h"
#include "Engine/Core/Debug.h"

#define Unused(var) var
#define MemZero(x) memset(&x, 0, sizeof(x));

template<typename T>
void Swap(T& a, T& b)
{
	T temp = a;
	a = b;
	b = temp;
}