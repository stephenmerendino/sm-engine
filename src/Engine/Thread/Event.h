#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Platform/WindowsInclude.h"

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