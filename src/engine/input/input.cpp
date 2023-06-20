#include "engine/input/input.h"
#include "engine/core/bit_flags.h"
#include "engine/core/debug.h"
#include "engine/core/macros.h"
#include "engine/math/ivec2.h"
#include "engine/render/window.h"
#include "engine/thirdparty/imgui/imgui.h"
#include <vcruntime_string.h>
#include <winuser.h>

enum class KeyStateBitFlags : u8
{
	IS_DOWN			= 0b000000001,
	WAS_PRESSED		= 0b000000010,
	WAS_RELEASED	= 0b000000100
};

struct key_state_t
{
    u8 state = 0;
};

static key_state_t s_key_states[(u32)KeyCode::NUM_KEY_CODES];
static vec2 s_mouse_movement_normalized = VEC2_ZERO;
static bool s_mouse_is_shown = true;
static bool s_unhide_mouse = false;
static bool s_hide_mouse = false;
static ivec2 s_saved_mouse_pos = IVEC2_ZERO;
static window_t* s_window = nullptr;

static 
void set_key_state_flag(key_state_t& key_state, KeyStateBitFlags flag, bool flag_value)
{
	if (flag_value)
	{
		set_bit(key_state.state, (u8)flag);
	}
	else
	{
		unset_bit(key_state.state, (u8)flag);
	}
}

static 
bool get_key_state_flag(const key_state_t& key_state, KeyStateBitFlags flag)
{
    return is_bit_set(key_state.state, (u8)flag);
}

static
bool get_key_state_index_from_win_key(int* out_index, u32 win_key)
{
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	
	// Numbers 0-9
	// 0x30 = 0, 0x39 = 9
	if (win_key >= 0x30 && win_key <= 0x39)
	{
		*out_index = (u32)KeyCode::KEY_0 + (win_key - 0x30);
		return true;
	}

	// Letters A-Z
	// 0x41 = A, 0x5A = Z
	if (win_key >= 0x41 && win_key <= 0x5A)
	{
		*out_index = (u32)KeyCode::KEY_A + (win_key - 0x41);
		return true;
	}

	// Numpad 0-9
	if (win_key >= VK_NUMPAD0 && win_key <= VK_NUMPAD9)
	{
		*out_index = (u32)KeyCode::KEY_NUMPAD0 + (win_key - VK_NUMPAD0);
		return true;
	}

	// F1-F24
	if (win_key >= VK_F1 && win_key <= VK_F24)
	{
		*out_index = (u32)KeyCode::KEY_F1 + (win_key - VK_F1);
		return true;
	}

	// Handle everything else directly
	switch (win_key)
	{
		case VK_LBUTTON:    *out_index = (u32)KeyCode::MOUSE_LBUTTON; return true;
		case VK_RBUTTON:    *out_index = (u32)KeyCode::MOUSE_RBUTTON; return true;
		case VK_MBUTTON:    *out_index = (u32)KeyCode::MOUSE_MBUTTON; return true;

		case VK_BACK:       *out_index = (u32)KeyCode::KEY_BACKSPACE; return true;
		case VK_TAB:        *out_index = (u32)KeyCode::KEY_TAB; return true;
		case VK_CLEAR:      *out_index = (u32)KeyCode::KEY_CLEAR; return true;
		case VK_RETURN:     *out_index = (u32)KeyCode::KEY_ENTER; return true;
		case VK_SHIFT:      *out_index = (u32)KeyCode::KEY_SHIFT; return true;
		case VK_CONTROL:    *out_index = (u32)KeyCode::KEY_CONTROL; return true;
		case VK_MENU:       *out_index = (u32)KeyCode::KEY_ALT; return true;
		case VK_PAUSE:      *out_index = (u32)KeyCode::KEY_PAUSE; return true;
		case VK_CAPITAL:    *out_index = (u32)KeyCode::KEY_CAPSLOCK; return true;
		case VK_ESCAPE:     *out_index = (u32)KeyCode::KEY_ESCAPE; return true;
		case VK_SPACE:      *out_index = (u32)KeyCode::KEY_SPACE; return true;
		case VK_PRIOR:      *out_index = (u32)KeyCode::KEY_PAGEUP; return true;
		case VK_NEXT:       *out_index = (u32)KeyCode::KEY_PAGEDOWN; return true;
		case VK_END:        *out_index = (u32)KeyCode::KEY_END; return true;
		case VK_HOME:       *out_index = (u32)KeyCode::KEY_HOME; return true;
		case VK_LEFT:       *out_index = (u32)KeyCode::KEY_LEFTARROW; return true;
		case VK_UP:         *out_index = (u32)KeyCode::KEY_UPARROW; return true;
		case VK_RIGHT:      *out_index = (u32)KeyCode::KEY_RIGHTARROW; return true;
		case VK_DOWN:       *out_index = (u32)KeyCode::KEY_DOWNARROW; return true;
		case VK_SELECT:     *out_index = (u32)KeyCode::KEY_SELECT; return true;
		case VK_PRINT:      *out_index = (u32)KeyCode::KEY_PRINT; return true;
		case VK_SNAPSHOT:   *out_index = (u32)KeyCode::KEY_PRINTSCREEN; return true;
		case VK_INSERT:     *out_index = (u32)KeyCode::KEY_INSERT; return true;
		case VK_DELETE:     *out_index = (u32)KeyCode::KEY_DELETE; return true;
		case VK_HELP:       *out_index = (u32)KeyCode::KEY_HELP; return true;
		case VK_SLEEP:      *out_index = (u32)KeyCode::KEY_SLEEP; return true;

		case VK_MULTIPLY:   *out_index = (u32)KeyCode::KEY_MULTIPLY; return true;
		case VK_ADD:        *out_index = (u32)KeyCode::KEY_ADD; return true;
		case VK_SEPARATOR:  *out_index = (u32)KeyCode::KEY_SEPARATOR; return true;
		case VK_SUBTRACT:   *out_index = (u32)KeyCode::KEY_SUBTRACT; return true;
		case VK_DECIMAL:    *out_index = (u32)KeyCode::KEY_DECIMAL; return true;
		case VK_DIVIDE:     *out_index = (u32)KeyCode::KEY_DIVIDE; return true;

		case VK_NUMLOCK:    *out_index = (u32)KeyCode::KEY_NUMLOCK; return true;
		case VK_SCROLL:     *out_index = (u32)KeyCode::KEY_SCROLLLOCK; return true;
	}

	return false;
}

static
void save_mouse_pos()
{
    POINT mouse_pos;
    ::GetCursorPos(&mouse_pos);
    s_saved_mouse_pos = make_ivec2(mouse_pos.x, mouse_pos.y);
}

static
void restore_mouse_pos()
{
    ::SetCursorPos(s_saved_mouse_pos.x, s_saved_mouse_pos.y);
}

static
void center_mouse_on_screen()
{
    ivec2 center_pos = window_get_center_position(s_window);
    ::SetCursorPos(center_pos.x, center_pos.y);
}

static
void update_mouse_movement()
{
    POINT mouse_pos;
    ::GetCursorPos(&mouse_pos);

    ivec2 center_pos = window_get_center_position(s_window);
    ivec2 window_size = make_ivec2(s_window->width, s_window->height);

    ivec2 delta = make_ivec2(mouse_pos.x, mouse_pos.y) - center_pos;

    s_mouse_movement_normalized = make_vec2((f32)delta.x / (f32)window_size.x,
                                            (f32)delta.y / (f32)window_size.y);
}

static
void hide_cursor()
{
    ::ShowCursor(false);
    s_mouse_is_shown = false;
}

static
void show_cursor()
{
    ::ShowCursor(true);
    s_mouse_is_shown = true;
}

static
void handle_win_key_down(u32 win_key)
{
	int key_state_index = -1;
	if (!get_key_state_index_from_win_key(&key_state_index, win_key))
	{
		return;
	}

	key_state_t& key_state = s_key_states[key_state_index];
	if (!get_key_state_flag(key_state, KeyStateBitFlags::IS_DOWN))
	{
        set_key_state_flag(key_state, KeyStateBitFlags::WAS_PRESSED, true);
	}
    set_key_state_flag(key_state, KeyStateBitFlags::IS_DOWN, true);
}

static
void handle_win_key_up(u32 win_key)
{
	int key_state_index = -1;
	if (!get_key_state_index_from_win_key(&key_state_index, win_key))
	{
		return;
	}

	key_state_t& key_state = s_key_states[key_state_index];
	if (get_key_state_flag(key_state, KeyStateBitFlags::IS_DOWN))
	{
        set_key_state_flag(key_state, KeyStateBitFlags::WAS_RELEASED, true);
	}
    set_key_state_flag(key_state, KeyStateBitFlags::IS_DOWN, false);
}

static
void reset_all_input_state()
{
    memset(s_key_states, 0, (u32)KeyCode::NUM_KEY_CODES * sizeof(key_state_t));
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static
bool imgui_msg_handler(UINT msg, WPARAM w_param, LPARAM l_param)
{
    if(!ImGui::GetCurrentContext()) return false;

    ImGuiIO& io = ImGui::GetIO();
    bool mouse_handled_by_imgui = io.WantSetMousePos || io.WantCaptureMouse;
    if(!mouse_handled_by_imgui)
    {
        // flag to unhide the mouse in input_update
        if (msg == WM_RBUTTONUP && !s_mouse_is_shown)
        {
            s_unhide_mouse = true; 
        }

        // flag to hide the mouse in input_update
        if(msg == WM_RBUTTONDOWN && s_mouse_is_shown)
        {
            s_hide_mouse = true;
        }
    }

    bool input_handled_by_imgui = ImGui_ImplWin32_WndProcHandler(s_window->handle, msg, w_param, l_param);
    return input_handled_by_imgui || mouse_handled_by_imgui;
}

static void input_system_msg_handler(UINT msg, WPARAM w_param, LPARAM l_param, void* user_args)
{
	UNUSED(l_param);
    UNUSED(user_args);

    if(imgui_msg_handler(msg, w_param, l_param))
    {
        reset_all_input_state();
        return;
    }

	switch (msg)
	{
		// Keyboard / mouse
		case WM_KEYDOWN:	    handle_win_key_down((u32)w_param); break;
		case WM_SYSKEYDOWN:     handle_win_key_down((u32)w_param); break;
		case WM_LBUTTONDOWN:    handle_win_key_down(VK_LBUTTON); break;
		case WM_MBUTTONDOWN:    handle_win_key_down(VK_MBUTTON); break;
		case WM_RBUTTONDOWN:    handle_win_key_down(VK_RBUTTON); break;
		
		case WM_KEYUP:		    handle_win_key_up((u32)w_param); break;
		case WM_SYSKEYUP:	    handle_win_key_up((u32)w_param); break;
		case WM_LBUTTONUP:      handle_win_key_up(VK_LBUTTON); break;
		case WM_MBUTTONUP:      handle_win_key_up(VK_MBUTTON); break;
		case WM_RBUTTONUP:      handle_win_key_up(VK_RBUTTON); break;
	}
}

void input_init(window_t* window)
{
    SM_ASSERT(nullptr != window);
    window_add_msg_callback(window, input_system_msg_handler);
    s_window = window;
}

void input_begin_frame()
{
    s_mouse_movement_normalized = VEC2_ZERO;
	for (int i = 0; i < (u32)KeyCode::NUM_KEY_CODES; i++)
	{
        set_key_state_flag(s_key_states[i], KeyStateBitFlags::WAS_PRESSED, false);
        set_key_state_flag(s_key_states[i], KeyStateBitFlags::WAS_RELEASED, false);
	}
}

void input_update()
{
	// Mouse movement update
    if (s_unhide_mouse)
    {
        s_unhide_mouse = false;
        show_cursor();
        restore_mouse_pos();
    }
    else if (s_hide_mouse)
    {
        s_hide_mouse = false;
        hide_cursor();
        save_mouse_pos();
        center_mouse_on_screen();
    }
    else if (input_is_key_down(KeyCode::MOUSE_RBUTTON))
    {
        update_mouse_movement();
        center_mouse_on_screen();
    }
}

bool input_is_key_down(KeyCode key)
{
    return is_bit_set(s_key_states[(u32)key].state, (u8)KeyStateBitFlags::IS_DOWN);
}

bool input_was_key_pressed(KeyCode key)
{
    return is_bit_set(s_key_states[(u32)key].state, (u8)KeyStateBitFlags::WAS_PRESSED);
}

bool input_was_key_released(KeyCode key)
{
    return is_bit_set(s_key_states[(u32)key].state, (u8)KeyStateBitFlags::WAS_RELEASED);
}

vec2 input_get_mouse_movement()
{
	return s_mouse_movement_normalized;
}

