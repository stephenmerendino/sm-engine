#pragma once

#include <cstring> //memset

#define UNUSED(var) var
#define MEM_ZERO(x) memset(&x, 0, sizeof(x));

#define CONCATENATE_DIRECT(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_DIRECT(s1, s2)
