#include "sm/io/input.h"
#include "sm/math/ivec2.h"
#include "sm/platform/win32/win32_include.h"

using namespace sm;

static bool s_mouse_is_shown = true;

void sm::input_show_mouse()
{
	::ShowCursor(true);
	s_mouse_is_shown = true;
}

void sm::input_hide_mouse()
{
	::ShowCursor(false);
	s_mouse_is_shown = false;
}

bool sm::input_is_mouse_shown()
{
	return s_mouse_is_shown;
}

ivec2_t sm::input_get_mouse_position()
{
	POINT mousePos;
	::GetCursorPos(&mousePos);
	return ivec2_t(mousePos.x, mousePos.y);
}

void sm::input_set_mouse_position(const ivec2_t& pos)
{
	::SetCursorPos(pos.x, pos.y);
}
