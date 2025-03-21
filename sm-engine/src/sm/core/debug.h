#pragma once

namespace sm
{
    inline constexpr bool is_running_in_debug()
    {
    #if defined(NDEBUG)
        return false;
    #else
        return true;
    #endif
    }

    void debug_printf(const char* format, ...);
}
