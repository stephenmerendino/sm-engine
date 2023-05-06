#pragma once

#include "engine/core/Types.h"
#include "engine/thread/AtomicInt.h"
#include "engine/thread/CriticalSection.h"

typedef void (*JobFunc)(void* args);

enum class JobStatus : U8
{
	CREATED,
	ENQUEUED,
	RUNNING,
	FINISHED,
	WAITING_ON_CHILDREN,
	NUM_JOB_STATUSES
};

class Job
{
public:
	explicit Job(JobFunc jobFunc, void* jobArgs);
	~Job();

	void Acquire();
	void Release();
	bool IsReleased() const;

	void Execute();
	void DependsOn(Job* dependency);
	void Finish();
	void DependencyFinished();

	bool IsWaitingOnDependencies();

	JobStatus GetJobStatus();
	void SetJobStatus(JobStatus status);

private:
	JobFunc m_jobFunc;
	void* m_jobArgs;
	Job* m_pParent;
	I32 m_childrenCount;
	JobStatus m_status;
	CriticalSection m_jobStatusRwGuard;
	AtomicInt m_refCount;
};
