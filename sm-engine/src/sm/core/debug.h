#pragma once

namespace sm::debug
{
    inline constexpr bool is_enabled()
    {
    #if defined(NDEBUG)
        return false;
    #else
        return true;
    #endif
    }

    void printf(const char* format, ...);
}
