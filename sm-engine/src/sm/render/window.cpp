#include "sm/render/window.h"

using namespace sm;

ivec2_t sm::window_calc_center_pos(const window_t& window)
{
	u32 half_width = window.width / 2;
	u32 half_height = window.height / 2;
	return ivec2_t(window.x + half_width, window.y + half_height);
}