#pragma once

#include "sm/core/types.h"

namespace sm
{
	struct window_t;

	void renderer_init(window_t* window);
    void renderer_begin_frame();
    void renderer_update_frame(f32 ds);
    void renderer_render_frame();
    void renderer_end_frame();
}
