#include "engine/job/JobSystem.h"
#include "engine/core/Common.h"
#include "engine/core/Assert.h"
#include "engine/thread/Event.h"
#include "engine/thread/ThreadSafeQueue.h"
#include "engine/math/MathUtils.h"

JobSystem g_jobSystem;

//*******************************

static Event s_jobsAddedEvent;
static ThreadSafeQueue<Job*> s_jobQueue;
static I64 s_numJobsSubmitted;
static I64 s_numJobsCompleted;
static bool s_shutdownJobSystem;
static CriticalSection s_shutdownJobSystemCs;
static CriticalSection s_numJobsProcessedCs;

// This function needs to pull jobs out of the job queue and execute them
DWORD WINAPI JobWorkerThreadFunc(void* args)
{
	Unused(args);

	while (true)
	{
		{
			SCOPED_CRITICAL_SECTION(&s_shutdownJobSystemCs);
			if (s_shutdownJobSystem && s_jobQueue.Empty())
			{
				break;
			}
		}

		Job* job = nullptr;
		if (s_jobQueue.Pop(&job))
		{
			job->Execute();
			job->Finish();
			g_jobSystem.ReleaseJob(job);

			SCOPED_CRITICAL_SECTION(&s_numJobsProcessedCs);
			s_numJobsCompleted++;
		}
		else
		{
			s_jobsAddedEvent.Wait();
		}
	}

	return 0;
}

//*******************************

JobSystem::JobSystem()
	:m_numWorkersThreads(0)
	, m_workerThreads(nullptr)
{
}

JobSystem::~JobSystem()
{
}

bool JobSystem::Init()
{
	// Determine how many threads we need to make based on cpu, leaving 1 thread for the main thread
	m_numWorkersThreads = std::thread::hardware_concurrency() - 1;

	// Handle case where we get 0 back for thread count
	m_numWorkersThreads = Max(1u, m_numWorkersThreads);

	// Allocate Threads
	m_workerThreads = new Thread[m_numWorkersThreads];

	// Start each Thread
	for (U32 i = 0; i < m_numWorkersThreads; ++i)
	{
		Thread& t = m_workerThreads[i];
		t.Run(JobWorkerThreadFunc);
	}

	return true;
}

void JobSystem::Shutdown()
{
	{
		SCOPED_CRITICAL_SECTION(&s_shutdownJobSystemCs);
		s_shutdownJobSystem = 1;
	}
	WakeUpWorkers();
	WaitAll();
}

Job* JobSystem::CreateJob(JobFunc func, void* args)
{
	Job* newJob = new Job(func, args);
	return newJob;
}

void JobSystem::SubmitJob(Job* job)
{
	ASSERT(job);

	job->Acquire();

	if (job->IsWaitingOnDependencies())
	{
		return;
	}

	job->SetJobStatus(JobStatus::ENQUEUED);

	s_jobQueue.Push(job);
	WakeUpWorkers();

	SCOPED_CRITICAL_SECTION(&s_numJobsProcessedCs);
	s_numJobsSubmitted++;
}

void JobSystem::ReleaseJob(Job* job)
{
	ASSERT(job);

	job->Release();
	if (job->IsReleased())
	{
		delete job;
	}
}

void JobSystem::SubmitAndReleaseJob(Job* job)
{
	SubmitJob(job);
	ReleaseJob(job);
}

void JobSystem::SubmitAndReleaseJob(JobFunc func, void* args)
{
	Job* newJob = CreateJob(func, args);
	SubmitAndReleaseJob(newJob);
}

bool JobSystem::IsBusy()
{
	SCOPED_CRITICAL_SECTION(&s_numJobsProcessedCs);
	return s_numJobsCompleted < s_numJobsSubmitted;
}

void JobSystem::WakeUpWorkers()
{
	s_jobsAddedEvent.SignalAndReset();
}

void JobSystem::WaitOnJob(Job* job)
{
	while (job->GetJobStatus() != JobStatus::FINISHED)
	{
		Thread::YieldExecution();
	}
}

void JobSystem::WaitAll()
{
	while (IsBusy())
	{
		Thread::YieldExecution();
	}
}
