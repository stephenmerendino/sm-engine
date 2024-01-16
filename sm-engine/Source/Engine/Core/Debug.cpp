#include "Engine/Core/Debug.h"
#include "Engine/Core/Assert.h"
#include "Engine/Platform/WindowsInclude.h"
//#include "Engine/Render/ui/ui.h"

#include <cstdio>

void DebugPrintf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	static const int MAX_BUFFER_SIZE = 2048;
	char formattedMsg[MAX_BUFFER_SIZE];
	SM_ASSERT(vsnprintf_s(formattedMsg, MAX_BUFFER_SIZE, format, args) != -1);
	va_end(args);

	OutputDebugStringA(formattedMsg);

	// TODO: Reenable this when we get imgui hooked back up
	// ui_log_msg_persistent(formattedMsg);
}
