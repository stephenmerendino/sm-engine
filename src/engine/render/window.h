#pragma once

#include "engine/core/types.h"
#include "engine/math/ivec2.h"
#include "engine/platform/windows_include.h"

#include <vector>
#include <string>

typedef void (*window_message_cb_t)(UINT msg, WPARAM w_param, LPARAM l_param, void* user_args);
struct window_message_cb_with_args_t
{
	window_message_cb_t m_cb;
	void* m_user_args;
};

struct window_t
{
    HWND m_handle;
    const char* m_name;
    u32 m_width;
    u32 m_height;
    u32 m_x;
    u32 m_y;
    bool m_is_minimized;
    bool m_was_resized;
    bool m_was_closed;
    std::vector<window_message_cb_with_args_t> m_message_cbs;
};

window_t*   window_create(const char* name, u32 width, u32 height, bool resizable);
void        window_update(window_t* window);
void        window_set_title(window_t* window, const char* new_title);
void        window_add_msg_callback(window_t* window, window_message_cb_t cb, void* args = nullptr);
ivec2       window_get_center_position(window_t* window);
void        window_destroy(window_t* window);
