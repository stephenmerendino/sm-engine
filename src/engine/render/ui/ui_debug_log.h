#pragma once
#include "engine/thirdparty/imgui/imgui.h"

struct ui_debug_log
{
    ImGuiTextBuffer text_buffer;
    ImGuiTextFilter text_filter;
    ImVector<int>   line_offsets;
    bool            auto_scroll = true;

    ui_debug_log();
    void clear();
    void log_msg(const char* msg);
    void log_msg_fmt(const char* fmt, ...);
    void log_msg_fmt(const char* fmt, va_list args);
    void draw();
};
