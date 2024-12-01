#include "sm/io/device_input.h"
#include "sm/render/window.h"

using namespace sm;

void input_handler(window_msg_type_t msg_type, u64 msg_data, void* user_args)
{

}

void sm::init_device_input(window_t* window)
{
	SM_ASSERT(window);
	add_window_msg_cb(window, input_handler, nullptr);
}