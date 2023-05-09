#pragma once

#include <cstring> //memset

#define UNUSED(var) var
#define MEM_ZERO(x) memset(&x, 0, sizeof(x));
