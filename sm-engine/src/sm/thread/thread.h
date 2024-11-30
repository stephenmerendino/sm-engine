#pragma once

#include "sm/core/types.h"

namespace sm
{
	void yield_thread();
	void sleep_thread_ms(f32 ms);
	void sleep_thread_seconds(f32 seconds);
}
