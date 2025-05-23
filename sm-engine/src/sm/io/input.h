#pragma once

#include "sm/core/types.h"
#include "sm/math/ivec2.h"
#include "sm/math/vec2.h"

namespace sm
{
	struct window_t;

    enum class key_code_t : u32
    {
        KEY_0,
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,

        KEY_A,
        KEY_B,
        KEY_C,
        KEY_D,
        KEY_E,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_I,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_M,
        KEY_N,
        KEY_O,
        KEY_P,
        KEY_Q,
        KEY_R,
        KEY_S,
        KEY_T,
        KEY_U,
        KEY_V,
        KEY_W,
        KEY_X,
        KEY_Y,
        KEY_Z,

        MOUSE_LBUTTON,
        MOUSE_RBUTTON,
        MOUSE_MBUTTON,

        KEY_BACKSPACE,
        KEY_TAB,
        KEY_CLEAR,
        KEY_ENTER,
        KEY_SHIFT,
        KEY_CONTROL,
        KEY_ALT,
        KEY_PAUSE,
        KEY_CAPSLOCK,
        KEY_ESCAPE,
        KEY_SPACE,
        KEY_PAGEUP,
        KEY_PAGEDOWN,
        KEY_END,
        KEY_HOME,
        KEY_LEFTARROW,
        KEY_UPARROW,
        KEY_RIGHTARROW,
        KEY_DOWNARROW,
        KEY_SELECT,
        KEY_PRINT,
        KEY_PRINTSCREEN,
        KEY_INSERT,
        KEY_DELETE,
        KEY_HELP,
        KEY_SLEEP,

        KEY_NUMPAD0,
        KEY_NUMPAD1,
        KEY_NUMPAD2,
        KEY_NUMPAD3,
        KEY_NUMPAD4,
        KEY_NUMPAD5,
        KEY_NUMPAD6,
        KEY_NUMPAD7,
        KEY_NUMPAD8,
        KEY_NUMPAD9,
        KEY_MULTIPLY,
        KEY_ADD,
        KEY_SEPARATOR,
        KEY_SUBTRACT,
        KEY_DECIMAL,
        KEY_DIVIDE,

        KEY_F1,
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_F11,
        KEY_F12,
        KEY_F13,
        KEY_F14,
        KEY_F15,
        KEY_F16,
        KEY_F17,
        KEY_F18,
        KEY_F19,
        KEY_F20,
        KEY_F21,
        KEY_F22,
        KEY_F23,
        KEY_F24,
        KEY_NUMLOCK,
        KEY_SCROLLLOCK,

        KEY_INVALID,

        NUM_KEY_CODES
    };

    enum class key_state_bit_flags_t : u8
    {
        IS_DOWN         = 0x01,
        WAS_PRESSED     = 0x02,
        WAS_RELEASED    = 0x04 
    };

	void input_init(window_t* window);
	void input_begin_frame();
	void input_update(f32 ds);

	bool input_is_key_down(key_code_t key);
	bool input_was_key_pressed(key_code_t key);
	bool input_was_key_released(key_code_t key);

    void input_show_mouse();
    void input_hide_mouse();
    bool input_is_mouse_shown();
    ivec2_t input_get_mouse_position();
    void input_set_mouse_position(const ivec2_t& pos);
    vec2_t input_get_mouse_movement_normalized();
}
