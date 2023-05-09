#pragma once

#include "engine/core/types.h"

void init_time();

struct stopwatch_t
{
    i64 m_start_tick = 0;   
};

void start_stopwatch(stopwatch_t& stopwatch);
f32 get_stopwatch_elapsed_seconds(const stopwatch_t& stopwatch);
f32 get_stopwatch_elapsed_ms(const stopwatch_t& stopwatch);
