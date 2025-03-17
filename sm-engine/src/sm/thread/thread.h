#pragma once

#include "sm/core/types.h"

namespace sm
{
	void thread_yield();
	void thread_sleep_ms(f32 ms);
	void thread_sleep_seconds(f32 seconds);
}
