#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Platform/WindowsInclude.h"

class Event
{
public:
	Event();

	void Init();
	void Destroy();

	void Wait();
	void WaitForMs(U32 ms);
	void Signal();
	void Reset();
	void SignalAndReset();

	HANDLE m_winEvent;
};
