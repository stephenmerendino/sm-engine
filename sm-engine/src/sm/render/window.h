#pragma once

#include "sm/core/types.h"
#include "sm/core/array.h"
#include "sm/math/ivec2.h"
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
        sm::array_t<window_msg_cb_with_args_t> msg_cbs;
    };

    window_t*   window_init(sm::arena_t* arena, const char* name, u32 width, u32 height, bool resizable);
    void        window_add_msg_cb(window_t* window, window_msg_cb_t cb, void* args);
    void        window_update(window_t* window);
    void        window_set_title(window_t* window, const char* new_title);
    ivec2_t     window_calc_center_pos(const window_t& window);
}
