#include "engine/thread/thread.h"
#include "engine/core/assert.h"
#include "engine/core/time.h"

thread_t create_thread(thread_func_t func)
{
    thread_t thread;
    thread.m_func = func;
    return thread;
}

void thread_run(thread_t t)
{
	if (t.m_is_running)
	{
		return;
	}

	t.m_handle = ::CreateThread(0, // Security attrs
                                0, // Stack size
                                t.m_func, // Function to run on new thread
                                NULL, // Parameter to pass to thread
                                0, // Creation flags
                                (LPDWORD)&t.m_id); // Out id

	ASSERT(NULL != t.m_handle);
    t.m_is_running = true;
}

thread_t create_and_run_thread(thread_func_t func)
{
    thread_t thread = create_thread(func); 
    thread_run(thread);
    return thread;
}

void thread_yield()
{
	::SwitchToThread();
}

void thread_sleep_ms(f32 ms)
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
    start_stopwatch(stopwatch);
	while (get_stopwatch_elapsed_ms(stopwatch) < ms) { thread_yield(); }
}

void thread_sleep_seconds(f32 seconds)
{
    thread_sleep_ms(seconds * 1000.0f);
}
