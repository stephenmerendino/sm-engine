#include "sm/core/time.h"
#include "sm/core/assert.h"
#include "sm/platform/win32/win32_include.h"

using namespace sm;

static i64 s_freq_per_sec = 0;

static i64 get_current_tick()
{
	LARGE_INTEGER tick;
	SM_ASSERT(::QueryPerformanceCounter(&tick));
	return tick.QuadPart;
}

void sm::time_init()
{
	static bool did_init = false;
	if (!did_init)
	{
		LARGE_INTEGER freq;
		BOOL res = ::QueryPerformanceFrequency(&freq);
		SM_ASSERT(res);
		s_freq_per_sec = freq.QuadPart;
		did_init = true;
	}
}

void sm::stopwatch_start(stopwatch_t* stopwatch)
{
	SM_ASSERT(stopwatch);
	stopwatch->start_tick = get_current_tick();
}

f32 sm::stopwatch_get_elapsed_seconds(stopwatch_t* stopwatch)
{
	SM_ASSERT(stopwatch);
	i64 cur_tick = get_current_tick();
	i64 ticks_since_start = cur_tick - stopwatch->start_tick;
	f32 seconds_elapsed = (f32)(ticks_since_start) / (f32)(s_freq_per_sec);
	return seconds_elapsed;
}

f32 sm::stopwatch_get_elapsed_ms(stopwatch_t* stopwatch)
{
	SM_ASSERT(stopwatch);
	return stopwatch_get_elapsed_seconds(stopwatch) * 1000.0f;
}
