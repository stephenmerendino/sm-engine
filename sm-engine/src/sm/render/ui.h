#pragma once

#include "sm/core/types.h"

namespace sm
{
	void ui_init();
	void ui_begin_frame();
	void ui_render();

	//todo: better communication design between ui and rest of engine
	typedef void (*build_scene_window_cb_t)();
	void ui_set_build_scene_window_callback(build_scene_window_cb_t cb);

	bool ui_was_reload_shaders_requested();
}
