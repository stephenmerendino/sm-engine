#include "engine/core/Debug.h"
#include "engine/core/Assert.h"
#include "engine/platform/windows_include.h"
#include "engine/render/ui/ui.h"

#include <cstdio>

void debug_printf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
        static const int MAX_BUFFER_SIZE = 2048;
        char formatted_msg[MAX_BUFFER_SIZE];
        SM_ASSERT(vsnprintf_s(formatted_msg, MAX_BUFFER_SIZE, format, args) != -1);
	va_end(args);

	OutputDebugStringA(formatted_msg);
    ui_log_msg(formatted_msg);
}
