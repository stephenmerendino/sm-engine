#include "Engine/Thread/Event.h"
#include "Engine/Core/Assert_old.h"
#include <synchapi.h>

Event::Event()
	:m_winEvent(NULL)
{
}

void Event::Init()
{
	m_winEvent = ::CreateEvent(NULL,  // security attributes
							   TRUE,  // is manual reset
							   FALSE, // initial state
							   NULL); // optional name
	SM_ASSERT_OLD(m_winEvent != NULL);
}

void Event::Destroy()
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