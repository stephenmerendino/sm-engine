#include "sm/io/device_input.h"
#include "sm/math/ivec2.h"
#include "sm/platform/win32/win32_include.h"

using namespace sm;

void sm::show_mouse()
{
	::ShowCursor(true);
}

void sm::hide_mouse()
{
	::ShowCursor(false);
}

ivec2_t sm::get_mouse_position()
{
	POINT mousePos;
	::GetCursorPos(&mousePos);
	return ivec2_t(mousePos.x, mousePos.y);
}

void sm::set_mouse_position(const ivec2_t& pos)
{
	::SetCursorPos(pos.x, pos.y);
}
