#pragma once

#include "sm/core/types.h"

namespace sm
{
	void time_init();

    struct stopwatch_t
    {
        i64 start_tick = 0;
    };

    void stopwatch_start(stopwatch_t* stopwatch);
    f32 stopwatch_get_elapsed_seconds(stopwatch_t* stopwatch);
    f32 stopwatch_get_elapsed_ms(stopwatch_t* stopwatch);
}
