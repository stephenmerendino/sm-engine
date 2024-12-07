#include "sm/io/device_input.h"
#include "sm/core/bits.h"
#include "sm/render/window.h"

using namespace sm;

u8 s_key_states[(u8)key_code_t::NUM_KEY_CODES] = { 0 };

void input_handler(window_msg_type_t msg_type, u64 msg_data, void* user_args)
{
	if(msg_type == window_msg_type_t::KEY_DOWN)
	{
		key_code_t key_code = (key_code_t)msg_data;
		if(key_code == key_code_t::KEY_INVALID)
		{
			return;
		}

		if(!is_bit_set((u8)s_key_states[(u32)key_code], (u8)key_state_bit_flags::IS_DOWN))
		{
			set_bit((u8&)s_key_states[(u32)key_code], (u8)key_state_bit_flags::WAS_PRESSED);
		}
        set_bit((u8&)s_key_states[(u32)key_code], (u8)key_state_bit_flags::IS_DOWN);
	}

	if(msg_type == window_msg_type_t::KEY_UP)
	{
		key_code_t key_code = (key_code_t)msg_data;
		if(key_code == key_code_t::KEY_INVALID)
		{
			return;
		}

		if(is_bit_set((u8)s_key_states[(u32)key_code], (u8)key_state_bit_flags::IS_DOWN))
		{
			set_bit((u8&)s_key_states[(u32)key_code], (u8)key_state_bit_flags::WAS_RELEASED);
		}
        unset_bit((u8&)s_key_states[(u32)key_code], (u8)key_state_bit_flags::IS_DOWN);
	}
}

void sm::init_device_inputs(window_t* window)
{
	SM_ASSERT(window);
	add_window_msg_cb(window, input_handler, nullptr);
}

void sm::begin_frame_device_inputs()
{
	for (int i = 0; i < (u32)key_code_t::NUM_KEY_CODES; i++)
	{
		unset_bit(s_key_states[i], (u8)key_state_bit_flags::WAS_PRESSED | (u8)key_state_bit_flags::WAS_RELEASED);
	}
}

void sm::update_device_inputs()
{
}

bool sm::is_key_down(key_code_t key)
{
	return is_bit_set(s_key_states[(u8)key], (u8)key_state_bit_flags::IS_DOWN);
}

bool sm::was_key_pressed(key_code_t key)
{
	return is_bit_set(s_key_states[(u8)key], (u8)key_state_bit_flags::WAS_PRESSED);
}

bool sm::was_key_released(key_code_t key)
{
	return is_bit_set(s_key_states[(u8)key], (u8)key_state_bit_flags::WAS_RELEASED);
}