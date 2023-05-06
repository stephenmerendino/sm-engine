#pragma once

inline constexpr bool IsDebug()
{
#if defined(NDEBUG)
	return false;
#else
	return true;
#endif
}

void DebugPrintf(const char* msgFormat, ...);