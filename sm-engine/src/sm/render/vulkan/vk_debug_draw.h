#pragma once

#include "sm/math/primitives.h"
#include "sm/core/color.h"
#include "sm/render/vulkan/vk_context.h"

namespace sm
{
	void debug_draw_init(render_context_t& context);
	void debug_draw_update();
	void debug_draw_sphere(const sphere_t& sphere, const color_f32_t& color, bool wireframe = false, u32 num_frames = 1);
};
