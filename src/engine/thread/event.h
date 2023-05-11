#pragma once

#include "engine/core/types.h"
#include "engine/platform/windows_include.h"

struct event_t
{
    HANDLE m_os_event;
};

event_t create_event();
void destroy_event(event_t e);
void wait(event_t e);
void wait_for_ms(event_t e, u32 ms);
void signal(event_t e);
void reset(event_t e);
void signal_and_reset(event_t e);
