#pragma once

#include "engine/core/types.h"
#include "engine/platform/windows_include.h"

struct event_t
{
    HANDLE m_os_event;
};

event_t event_create();
void event_destroy(event_t e);
void event_wait(event_t e);
void event_wait_for_ms(event_t e, u32 ms);
void event_signal(event_t e);
void event_reset(event_t e);
void event_signal_and_reset(event_t e);
