#pragma once

#define STR_PRINTF_BOOL(bool_val) (bool_val ? "True" : "False")

inline constexpr bool is_debug()
{
#if defined(NDEBUG)
	return false;
#else
	return true;
#endif
}

void debug_printf(const char* format, ...);
