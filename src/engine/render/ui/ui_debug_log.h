#pragma once
#include "engine/thirdparty/imgui/imgui.h"

void ui_log_init();
void ui_log_begin_frame();
void ui_log_clear_persistent();
void ui_log_clear_frame();
void ui_log_msg_persistent(const char* msg);
void ui_log_msg_persistent_fmt(const char* fmt, ...);
void ui_log_msg_persistent_fmt(const char* fmt, va_list args);
void ui_log_msg_frame(const char* msg);
void ui_log_msg_frame_fmt(const char* fmt, ...);
void ui_log_msg_frame_fmt(const char* fmt, va_list args);
void ui_log_draw();
