#pragma once

#include "Engine/Thread/AtomicUtil.h"
#include "Engine/Thread/CriticalSection.h"
#include "Engine/Thread/Thread.h"

typedef void (*JobFunc)(void* args);

enum class JobStatus : U8
{
	CREATED,
	ENQUEUED,
	RUNNING,
	FINISHED,
	WAITING,
	NUM_JOB_STATUSES
};

struct Job
{
	JobFunc m_func;
	void* m_args;
	Job* m_jobWaitingOnMe;
	I32 m_waitingOnCount;
	JobStatus m_status;
	CriticalSection m_jobStatusRwLock;
	AtomicInt m_refCount;
};

class JobSystem
{
public:
	bool Init();
	void Shutdown();
};

extern JobSystem g_jobSystem;

//void job_system_init();
//void job_system_shutdown();
//job_t* job_system_create_job(job_func_t func, void* args = nullptr);
//void job_system_submit_job(job_t* job);
//void job_system_release_job(job_t* job);
//void job_system_submit_and_release_job(job_t* job);
//void job_system_submit_and_release_job(job_func_t func, void* args = nullptr);
//bool job_system_is_busy();
//void job_system_wait_all();
//void job_system_wait_on_job(job_t* job);
//
//void job_wait_on(job_t* job, job_t* job_to_wait_for);
//JobStatus job_get_status(job_t* job);
