#pragma once

#include "engine/core/Types.h"
#include "engine/platform/WindowsInclude.h"

class Event
{
public:
	Event();
	~Event();

	void Wait();
	void WaitForMs(U32 ms);
	void Signal();
	void Reset();
	void SignalAndReset();

private:
	HANDLE m_osEvent;
};
