#include "engine/thread/event.h"
#include <synchapi.h>

event_t create_event()
{
    event_t event;
	event.m_os_event = ::CreateEvent(NULL,	// security attributes
                                     TRUE,  // is manual reset
                                     FALSE, // initial state
                                     NULL); // optional name
    return event;
}

void destroy_event(event_t e)
{
	::CloseHandle(e.m_os_event);
}

void wait(event_t e)
{
	::WaitForSingleObject(e.m_os_event, INFINITE);
}

void wait_for_ms(event_t e, u32 ms)
{
	::WaitForSingleObject(e.m_os_event, (DWORD)ms);
}

void signal(event_t e)
{
	::SetEvent(e.m_os_event);
}

void reset(event_t e)
{
	::ResetEvent(e.m_os_event);
}

void signal_and_reset(event_t e)
{
    signal(e);
    reset(e);
}
