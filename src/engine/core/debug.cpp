#include "engine/core/Debug.h"
#include "engine/core/Assert.h"
#include "engine/platform/windows_include.h"

#include <cstdio>

void debug_printf(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	static const int MAX_BUFFER_SIZE = 2048;
	char formatted_msg[MAX_BUFFER_SIZE];
	ASSERT(vsnprintf_s(formatted_msg, MAX_BUFFER_SIZE, format, args) != -1);

	OutputDebugStringA(formatted_msg);

	va_end(args);
}
