#pragma once

namespace sm
{
    inline constexpr bool is_enabled()
    {
    #if defined(NDEBUG)
        return false;
    #else
        return true;
    #endif
    }

    void debug_printf(const char* format, ...);
}
