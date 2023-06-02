#include "engine/core/time.h"
#include "engine/core/Assert.h"
#include "engine/platform/windows_include.h"

static i64 s_freq_per_sec;

static i64 get_current_tick()
{
	LARGE_INTEGER tick;
	SM_ASSERT(QueryPerformanceCounter(&tick));
	return tick.QuadPart;
}

void time_init()
{
	static bool did_init = false;
	if (!did_init)
	{
		LARGE_INTEGER freq;
		BOOL res = QueryPerformanceFrequency(&freq);
		SM_ASSERT(res);
		s_freq_per_sec = freq.QuadPart;
		did_init = true;
	}
}

void stopwatch_start(stopwatch_t& stopwatch)
{
   stopwatch.m_start_tick = get_current_tick();
}

f32 stopwatch_get_elapsed_seconds(const stopwatch_t& stopwatch)
{
	i64 cur_tick = get_current_tick();
	i64 ticks_since_start = cur_tick - stopwatch.m_start_tick;
	f32 seconds_elapsed = (f32)(ticks_since_start) / (f32)(s_freq_per_sec);
	return seconds_elapsed;
}

f32 stopwatch_get_elapsed_ms(const stopwatch_t& stopwatch)
{
	return stopwatch_get_elapsed_seconds(stopwatch) * 1000.0f;
}
