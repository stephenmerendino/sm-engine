#include "sm/core/debug.h"
#include "sm/core/assert.h"
#include "sm/platform/win32/win32_include.h"

#include <cstdio>

void sm::debug::printf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	static const int MAX_BUFFER_SIZE = 2048;
	char formatted_msg[MAX_BUFFER_SIZE];
	SM_ASSERT(vsnprintf_s(formatted_msg, MAX_BUFFER_SIZE, format, args) != -1);
	va_end(args);

	OutputDebugStringA(formatted_msg);
}
