#pragma once

#include "engine/thread/atomic_util.h"
#include "engine/thread/crtical_section.h"
#include "engine/thread/thread.h"

typedef void (*job_func_t)(void* args);

enum class JobStatus : u8
{
	CREATED,
	ENQUEUED,
	RUNNING,
	FINISHED,
	WAITING,
	NUM_JOB_STATUSES
};

struct job_t
{
    job_func_t m_func;
    void* m_args;
    job_t* m_job_waiting_on_me;
    i32 m_waiting_on_count;
    JobStatus m_status;
    critical_section_t m_job_status_rw_cs;
    atomic_int_t m_ref_count;
};

void job_system_init();
void job_system_shutdown();
job_t* job_system_create_job(job_func_t func, void* args = nullptr);
void job_system_submit_job(job_t* job);
void job_system_release_job(job_t* job);
void job_system_submit_and_release_job(job_t* job);
void job_system_submit_and_release_job(job_func_t func, void* args = nullptr);
bool job_system_is_busy();
void job_system_wait_all();
void job_system_wait_on_job(job_t* job);

void job_wait_on(job_t* job, job_t* job_to_wait_for);
JobStatus job_get_status(job_t* job);
