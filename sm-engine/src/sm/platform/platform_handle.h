#pragma once

#include "sm/core/types.h"

namespace sm
{
    struct platform_handle_t
    {
        u64 data = 0;
    };

    template<typename T>
    void set_handle(platform_handle_t& handle, const T& raw_handle)
    {
        handle.data = *((u64*)&raw_handle);
    }

    template<typename T>
    T get_handle(const platform_handle_t& handle)
    {
        return *((T*)(&handle.data));
    }
}
