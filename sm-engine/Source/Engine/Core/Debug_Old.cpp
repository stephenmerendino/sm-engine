#include "Engine/Core/Debug_Old.h"
#include "Engine/Core/Assert_old.h"
#include "Engine/Platform/WindowsInclude.h"
#include "Engine/Render/UI/UI.h"

#include <cstdio>

void DebugPrintf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	static const int MAX_BUFFER_SIZE = 2048;
	char formattedMsg[MAX_BUFFER_SIZE];
	SM_ASSERT_OLD(vsnprintf_s(formattedMsg, MAX_BUFFER_SIZE, format, args) != -1);
	va_end(args);

	OutputDebugStringA(formattedMsg);

	UI::LogMsg(UI::MsgType::kPersistent, formattedMsg);
}
