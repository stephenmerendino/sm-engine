#include "engine/thread/event.h"
#include <synchapi.h>

event_t event_create()
{
    event_t event;
	event.m_os_event = ::CreateEvent(NULL,	// security attributes
                                     TRUE,  // is manual reset
                                     FALSE, // initial state
                                     NULL); // optional name
    return event;
}

void event_destroy(event_t e)
{
	::CloseHandle(e.m_os_event);
}

void event_wait(event_t e)
{
	::WaitForSingleObject(e.m_os_event, INFINITE);
}

void event_wait_for_ms(event_t e, u32 ms)
{
	::WaitForSingleObject(e.m_os_event, (DWORD)ms);
}

void event_signal(event_t e)
{
	::SetEvent(e.m_os_event);
}

void event_reset(event_t e)
{
	::ResetEvent(e.m_os_event);
}

void event_signal_and_reset(event_t e)
{
    event_signal(e);
    event_reset(e);
}
