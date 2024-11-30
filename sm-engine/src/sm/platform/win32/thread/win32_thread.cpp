#include "sm/thread/thread.h"
#include "sm/core/time.h"
#include "sm/platform/win32/win32_include.h"

using namespace sm;

void sm::yield_thread()
{
	::SwitchToThread();
}

void sm::sleep_thread_ms(f32 ms)
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
	start_stopwatch(&stopwatch);
	while (get_elapsed_ms(&stopwatch) < ms) { yield_thread(); }
}

void sm::sleep_thread_seconds(f32 seconds)
{
	sleep_thread_ms(seconds * 1000.0f);
}