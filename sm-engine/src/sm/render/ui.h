#pragma once

#include "sm/core/types.h"

namespace sm
{
	void ui_init();
	void ui_begin_frame();
	void ui_render();

	//todo: better communication design between ui and rest of engine
	bool ui_was_reload_shaders_requested();
}
