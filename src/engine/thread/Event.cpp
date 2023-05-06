#include "Engine/Thread/Event.h"

Event::Event()
	:m_osEvent(NULL)
{
	::CreateEvent(NULL,	 // security attributes
		TRUE,  // is manual reset
		FALSE, // initial state
		NULL); // optional name
}

Event::~Event()
{
	::CloseHandle(m_osEvent);
}

void Event::Wait()
{
	::WaitForSingleObject(m_osEvent, INFINITE);
}

void Event::WaitForMs(U32 ms)
{
	::WaitForSingleObject(m_osEvent, (DWORD)ms);
}

void Event::Signal()
{
	::SetEvent(m_osEvent);
}

void Event::Reset()
{
	::ResetEvent(m_osEvent);
}

void Event::SignalAndReset()
{
	Signal();
	Reset();
}