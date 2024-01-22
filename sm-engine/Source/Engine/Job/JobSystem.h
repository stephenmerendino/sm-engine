#pragma once

#include "Engine/Thread/AtomicUtil.h"
#include "Engine/Thread/CriticalSection.h"
#include "Engine/Thread/Event.h"
#include "Engine/Thread/Thread.h"
#include "Engine/Thread/ThreadSafeQueue.h"

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

class Job
{
public:
	void Init(JobFunc func, void* args);
	void Acquire();
	void Release();
	bool IsReleased() const;
	JobStatus GetStatus();
	void SetStatus(JobStatus status);
	void Execute();
	void Finish();
	bool IsWaitingOnOthers();
	void JobBeingWaitedOnIsFinished();
	void WaitOn(Job* job);

private:
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
	JobSystem();
	void Init();
	void Shutdown();

	void SubmitJob(Job* job);
	Job* SubmitJob(JobFunc func, void* args);
	void ReleaseJob(Job* job);
	void SubmitAndReleaseJob(Job* job);
	void SubmitAndReleaseJob(JobFunc func, void* args);
	void WaitOnJob(Job* job);
	void WaitOnAllJobs();
	bool IsBusy();

	CriticalSection m_shutdownJobSystemCs;
	CriticalSection m_numJobsSubmittedCs;
	CriticalSection m_numJobsCompletedCs;
	U32 m_numWorkerThreads;
	Thread* m_workerThreads;
	bool m_bShouldShutdown;
	ThreadSafeQueue<Job*> m_jobQueue;
	Event m_jobAddedEvent;
	I32 m_numJobsSubmitted;
	I32 m_numJobsCompleted;
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
