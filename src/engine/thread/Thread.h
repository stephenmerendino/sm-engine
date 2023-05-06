#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Platform/WindowsInclude.h"

#include <thread>

typedef LPTHREAD_START_ROUTINE ThreadFunc;

#define kInvalidThreadId (U32)-1

class Thread
{
public:
	Thread();
	~Thread();

	bool Run(ThreadFunc threadFunc);
	U32 GetThreadId() const;

	static bool YieldExecution();
	static void CoarseSleepMs(F32 ms);
	static void PreciseSleepMs(F32 ms);

private:
	bool m_bIsRunning;
	ThreadFunc m_threadFunc;
	HANDLE m_threadHandle;
	U32 m_threadId;
};