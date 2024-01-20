#include "Engine/Thread/Event.h"
#include <synchapi.h>

Event::Event()
	:m_winEvent(NULL)
{
	m_winEvent = ::CreateEvent(NULL,  // security attributes
							   TRUE,  // is manual reset
							   FALSE, // initial state
							   NULL); // optional name
}

Event::~Event()
{
	::CloseHandle(m_winEvent);
}

void Event::Wait()
{
	::WaitForSingleObject(m_winEvent, INFINITE);
}

void Event::WaitForMs(U32 ms)
{
	::WaitForSingleObject(m_winEvent, (DWORD)ms);
}

void Event::Signal()
{
	::SetEvent(m_winEvent);
}

void Event::Reset()
{
	::ResetEvent(m_winEvent);
}

void Event::SignalAndReset()
{
	Signal();
	Reset();
}