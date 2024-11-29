#include "Engine/Thread/Thread.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Time_old.h"

Thread::Thread()
	:m_func(NULL)
	,m_handle(NULL)
	,m_id(Thread::kInvalidThreadId)
	,m_bIsRunning(false)
{
}

void Thread::Run(ThreadFunc func)
{
	SM_ASSERT(!m_bIsRunning);

	m_func = func;

	m_handle = ::CreateThread(0, // Security attrs
							  0, // Stack size
							  m_func, // Function to run on new thread
							  NULL, // Parameter to pass to thread
							  0, // Creation flags
							  (LPDWORD)&m_id); // Out id

	SM_ASSERT(NULL != m_handle);
	m_bIsRunning = true;
}

void Thread::Yield()
{
	::SwitchToThread();
}

void Thread::SleepMs(F32 ms)
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
	Stopwatch stopwatch;
	stopwatch.Start();
	while (stopwatch.GetElapsedMs() < ms) { Yield(); }
}

void Thread::SleepSeconds(F32 seconds)
{
	SleepMs(seconds * 1000.0f);
}
