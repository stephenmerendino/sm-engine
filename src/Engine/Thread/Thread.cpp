#include "Engine/Thread/Thread.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Clock.h"

Thread::Thread()
	:m_bIsRunning(false)
	, m_threadFunc(nullptr)
	, m_threadHandle(nullptr)
	, m_threadId(kInvalidThreadId)
{
}

Thread::~Thread()
{
}

bool Thread::Run(ThreadFunc threadFunc)
{
	if (m_bIsRunning)
	{
		return false;
	}

	m_threadFunc = threadFunc;
	m_threadHandle = ::CreateThread(0, 0, m_threadFunc, this, 0, (LPDWORD)&m_threadId);

	ASSERT(m_threadHandle != NULL);
	m_bIsRunning = true;

	return true;
}

bool Thread::YieldExecution()
{
	return ::SwitchToThread();
}

void Thread::CoarseSleepMs(F32 ms)
{
	::Sleep(static_cast<DWORD>(ms));
}

void Thread::PreciseSleepMs(F32 ms)
{
	while (ms > 1000.0f)
	{
		Thread::CoarseSleepMs(1000);
		ms -= 1000.0f;
	}

	while (ms > 100.0f)
	{
		Thread::CoarseSleepMs(100);
		ms -= 100.0f;
	}

	// Spin down the last bit of time since calling the Win32 Sleep function is too inaccurate
	Clock stopWatch;
	stopWatch.Start();
	while (stopWatch.GetMsElapsed() < ms) { Thread::YieldExecution(); }
}

U32 Thread::GetThreadId() const
{
	return m_threadId;
}
