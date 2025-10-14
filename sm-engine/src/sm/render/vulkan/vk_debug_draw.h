#pragma once

#include "sm/math/primitives.h"
#include "sm/render/vulkan/vk_context.h"

namespace sm
{
	void debug_draw_init(render_context_t& context);
	void debug_draw_update();
	void debug_draw_sphere(const sphere_t& sphere, u32 num_frames = UINT32_MAX);
};
