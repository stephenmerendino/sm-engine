#pragma once

#define BOOL_TO_STRING(bool_val) (bool_val ? "True" : "False")

inline constexpr bool IsDebug()
{
#if defined(NDEBUG)
	return false;
#else
	return true;
#endif
}

void DebugPrintf(const char* format, ...);
