#pragma once

#include "sm/core/types.h"
#include "sm/core/array.h"
#include "sm/platform/platform_handle.h"

namespace sm
{
    struct arena_t;
}

namespace sm
{
    enum class window_msg_type_t
    {
        UNKNOWN = -1,
        KEY_UP,
        KEY_DOWN,
        CLOSE_WINDOW
    };

    typedef void (*window_msg_cb_t)(window_msg_type_t msg_type, u64 msg_data, void* user_args);
    struct window_msg_cb_with_args_t 
    {
        window_msg_cb_t cb	= nullptr;
        void* args			= nullptr;
    };

    struct window_t
    {
        platform_handle_t handle;
        const char* name;
        u32 width;
        u32 height;
        u32 x;
        u32 y;
        bool is_minimized;
        bool was_resized;
        bool was_closed;
        bool is_moving;
        sm::static_array_t<window_msg_cb_with_args_t> msg_cbs;
        u8 num_cbs;
        byte_t padding[3];
    };

    window_t* init_window(sm::arena_t* arena, const char* name, u32 width, u32 height, bool resizable);
    void add_window_msg_cb(window_t* window, window_msg_cb_t cb, void* args);
    void update_window(window_t* window);
    void set_title(window_t* window, const char* new_title);
}
