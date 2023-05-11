#pragma once

#include "engine/core/types.h"
#include "engine/platform/windows_include.h"

#include <thread>

typedef LPTHREAD_START_ROUTINE thread_func_t;

#define kInvalidThreadId (u32)-1

struct thread_t
{
	thread_func_t m_func    = NULL;
	HANDLE m_handle         = NULL;
	u32 m_id                = kInvalidThreadId;
	bool m_is_running       = false;
};

thread_t create_thread(thread_func_t func);
thread_t create_and_run_thread(thread_func_t func);
void thread_run(thread_t t);
void thread_yield();
void thread_sleep_ms(f32 ms);
void thread_sleep_seconds(f32 seconds);
