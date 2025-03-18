#include "sm/io/input.h"
#include "sm/core/bits.h"
#include "sm/render/window.h"

using namespace sm;

static window_t* s_window = nullptr;

u8 s_key_states[(u8)key_code_t::NUM_KEY_CODES] = { 0 };

vec2_t s_mouse_movement_normalized = vec2_t::ZERO;
bool s_mouse_is_shown = true;
bool s_unhide_mouse = false;
bool s_hide_mouse = false;
ivec2_t s_saved_mouse_pos = ivec2_t::ZERO;

static void save_mouse_pos()
{
	s_saved_mouse_pos = input_get_mouse_position();
}

static void restore_mouse_pos()
{
	input_set_mouse_position(s_saved_mouse_pos);
}

static void center_mouse_on_screen()
{
	ivec2_t center_mouse_pos = window_calc_center_pos(*s_window);
	input_set_mouse_position(center_mouse_pos);
}

static void update_mouse_movement()
{
	ivec2_t mouse_pos = input_get_mouse_position();

	ivec2_t center_window_pos = window_calc_center_pos(*s_window);
	ivec2_t window_size = ivec2_t(s_window->width, s_window->height);

	ivec2_t delta = mouse_pos - center_window_pos;

	s_mouse_movement_normalized = vec2_t((f32)delta.x / (f32)window_size.x, 
                                         (f32)delta.y / (f32)window_size.y);
}

void input_handler(window_msg_type_t msg_type, u64 msg_data, void* user_args)
{
	if(msg_type == window_msg_type_t::KEY_DOWN)
	{
		key_code_t msg_key_code = (key_code_t)msg_data;
		if(msg_key_code == key_code_t::KEY_INVALID)
		{
			return;
		}

		if(!is_bit_set((u8)s_key_states[(u32)msg_key_code], (u8)key_state_bit_flags_t::IS_DOWN))
		{
			set_bit((u8&)s_key_states[(u32)msg_key_code], (u8)key_state_bit_flags_t::WAS_PRESSED);
		}
        set_bit((u8&)s_key_states[(u32)msg_key_code], (u8)key_state_bit_flags_t::IS_DOWN);
	}

	if(msg_type == window_msg_type_t::KEY_UP)
	{
		key_code_t msg_key_code = (key_code_t)msg_data;
		if(msg_key_code == key_code_t::KEY_INVALID)
		{
			return;
		}

		if(is_bit_set((u8)s_key_states[(u32)msg_key_code], (u8)key_state_bit_flags_t::IS_DOWN))
		{
			set_bit((u8&)s_key_states[(u32)msg_key_code], (u8)key_state_bit_flags_t::WAS_RELEASED);
		}
        unset_bit((u8&)s_key_states[(u32)msg_key_code], (u8)key_state_bit_flags_t::IS_DOWN);
	}
}

void sm::input_init(window_t* window)
{
	SM_ASSERT(window);
	s_window = window;
	window_add_msg_cb(window, input_handler, nullptr);
}

void sm::input_begin_frame()
{
	for (int i = 0; i < (u32)key_code_t::NUM_KEY_CODES; i++)
	{
		unset_bit(s_key_states[i], (u8)key_state_bit_flags_t::WAS_PRESSED | (u8)key_state_bit_flags_t::WAS_RELEASED);
	}
}

void sm::input_update(f32 ds)
{
	// If imgui isn't being used then we check for right mouse hiding directly here, otherwise its in the imgui handler
	//if (!ImGui::GetCurrentContext())
	{
		if (!input_is_key_down(key_code_t::MOUSE_RBUTTON) && !s_mouse_is_shown)
		{
			s_unhide_mouse = true;
		}

		if (input_is_key_down(key_code_t::MOUSE_RBUTTON) && s_mouse_is_shown)
		{
			s_hide_mouse = true;
		}
	}

	// Mouse movement update
	if (s_unhide_mouse)
	{
		s_unhide_mouse = false;
		input_show_mouse();
		restore_mouse_pos();
	}
	else if (s_hide_mouse)
	{
		s_hide_mouse = false;
		input_hide_mouse();
		save_mouse_pos();
		center_mouse_on_screen();
	}
	else if (input_is_key_down(key_code_t::MOUSE_RBUTTON))
	{
		update_mouse_movement();
		center_mouse_on_screen();
	}
}

bool sm::input_is_key_down(key_code_t key)
{
	return is_bit_set(s_key_states[(u8)key], (u8)key_state_bit_flags_t::IS_DOWN);
}

bool sm::input_was_key_pressed(key_code_t key)
{
	return is_bit_set(s_key_states[(u8)key], (u8)key_state_bit_flags_t::WAS_PRESSED);
}

bool sm::input_was_key_released(key_code_t key)
{
	return is_bit_set(s_key_states[(u8)key], (u8)key_state_bit_flags_t::WAS_RELEASED);
}

vec2_t sm::input_get_mouse_movement_normalized()
{
	return s_mouse_movement_normalized;
}