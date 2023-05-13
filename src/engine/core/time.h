#pragma once

#include "engine/core/types.h"

void time_init();

struct stopwatch_t
{
    i64 m_start_tick = 0;   
};

void stopwatch_start(stopwatch_t& stopwatch);
f32 stopwatch_get_elapsed_seconds(const stopwatch_t& stopwatch);
f32 stopwatch_get_elapsed_ms(const stopwatch_t& stopwatch);
