#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Platform/WindowsInclude.h"
#include <thread>

#undef Yield // Something in Windows #defines this and I want to use it as a member function

typedef LPTHREAD_START_ROUTINE ThreadFunc;

class Thread
{
public:
	Thread();

	void Run(ThreadFunc func);

	static void Yield();
	static void SleepMs(F32 ms);
	static void SleepSeconds(F32 seconds);

	ThreadFunc m_func;
	HANDLE m_handle;
	U32 m_id;
	bool m_bIsRunning;

	enum : U32
	{
		kInvalidThreadId = (U32)-1
	};
};
