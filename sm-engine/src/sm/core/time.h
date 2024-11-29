#pragma once

#include "sm/core/types.h"

namespace sm
{
	void init_time();

    struct stopwatch_t
    {
        i64 start_tick = 0;
    };

    void start_stopwatch(stopwatch_t* stopwatch);
    f32 get_elapsed_seconds(stopwatch_t* stopwatch);
    f32 get_elapsed_ms(stopwatch_t* stopwatch);
}
