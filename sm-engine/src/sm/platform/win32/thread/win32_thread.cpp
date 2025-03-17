#include "sm/thread/thread.h"
#include "sm/core/time.h"
#include "sm/platform/win32/win32_include.h"

using namespace sm;

void sm::thread_yield()
{
	::SwitchToThread();
}

void sm::thread_sleep_ms(f32 ms)
{
	while (ms > 1000.0f)
	{
		::Sleep((DWORD)ms);
		ms -= 1000.0f;
	}

	while (ms > 100.0f)
	{
		::Sleep((DWORD)ms);
		ms -= 100.0f;
	}

	// Spin down the last bit of time since calling the Win32 Sleep function is too inaccurate
	stopwatch_t stopwatch;
	stopwatch_start(&stopwatch);
	while (stopwatch_get_elapsed_ms(&stopwatch) < ms) { thread_yield(); }
}

void sm::thread_sleep_seconds(f32 seconds)
{
	thread_sleep_ms(seconds * 1000.0f);
}