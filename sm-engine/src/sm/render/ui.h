#pragma once

#include "sm/core/types.h"

namespace sm
{
	enum class log_msg_type_t : u8
	{
		PERSISTENT,
		FRAME
	};

	void ui_init();
	void ui_begin_frame();
	void ui_render();

	void ui_log_msg(log_msg_type_t msgType, const char* msg);
	void ui_log_msg_format(log_msg_type_t msgType, const char* fmt, ...);
	void ui_clear_msg_log(log_msg_type_t msgType);
}
