#pragma once

#include "sm/math/primitives.h"

namespace sm
{
	void debug_draw_init();
	void debug_draw_update();
	void debug_draw_sphere(const sphere_t& sphere, u32 num_frames = UINT32_MAX);
};
