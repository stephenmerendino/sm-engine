#include "Engine/Core/Debug.h"
#include "Engine/Core/Assert.h"
#include "Engine/Platform/WindowsInclude.h"

#include <cstdio>

void DebugPrintf(const char* msgFormat, ...)
{
	va_list args;
	va_start(args, msgFormat);

	static const int MAX_BUFFER_SIZE = 2048;
	char formattedMsg[MAX_BUFFER_SIZE];
	ASSERT(vsnprintf_s(formattedMsg, MAX_BUFFER_SIZE, msgFormat, args) != -1);

	OutputDebugStringA(formattedMsg);

	va_end(args);
}