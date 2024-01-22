#include "Engine/Job/JobSystem.h"
#include "Engine/Core/Assert.h"
#include "Engine/Math/MathUtils.h"

JobSystem g_jobSystem;

// this function needs to pull jobs out of the job queue and execute them
static DWORD WINAPI JobWorkerThreadFunc(void* args)
{
	UNUSED(args);

	while (true)
	{
		{
			SCOPED_CRITICAL_SECTION(&g_jobSystem.m_shutdownJobSystemCs);
			if (g_jobSystem.m_bShouldShutdown && g_jobSystem.m_jobQueue.Empty())
			{
				break;
			}
		}

		Job* job = nullptr;
		if (g_jobSystem.m_jobQueue.Pop(&job))
		{
			job->Execute();
			job->Finish();
			g_jobSystem.ReleaseJob(job);

			SCOPED_CRITICAL_SECTION(&g_jobSystem.m_numJobsCompletedCs);
			g_jobSystem.m_numJobsCompleted++;
		}
		else
		{
			g_jobSystem.m_jobAddedEvent.Wait();
		}
	}

	return 0;
}

void Job::Init(::JobFunc func, void* args)
{
	m_func = func;
	m_args = args;
	m_jobWaitingOnMe = nullptr;
	m_waitingOnCount = 0;
	m_status = JobStatus::CREATED;
	m_jobStatusRwLock.Init();
	m_refCount = 1;
}

void Job::Acquire()
{
	AtomicIncr(&m_refCount);
}

void Job::Release()
{
	AtomicDecr(&m_refCount);
}

bool Job::IsReleased() const
{
	return m_refCount == 0;
}

JobStatus Job::GetStatus()
{
	SCOPED_CRITICAL_SECTION(&m_jobStatusRwLock);
	return m_status;
}

void Job::SetStatus(JobStatus status)
{
	m_jobStatusRwLock.Lock();
	m_status = status;
}

void Job::Execute()
{
	SetStatus(JobStatus::RUNNING);
	m_func(m_args);
}

void Job::Finish()
{
	SetStatus(JobStatus::FINISHED);
	if (m_jobWaitingOnMe)
	{
		m_jobWaitingOnMe->JobBeingWaitedOnIsFinished();
	}
}

bool Job::IsWaitingOnOthers()
{
	return GetStatus() == JobStatus::WAITING;
}

void Job::JobBeingWaitedOnIsFinished()
{
	m_waitingOnCount--;
	g_jobSystem.SubmitJob(this);
}

void Job::WaitOn(Job* job)
{
	m_waitingOnCount++;
	job->m_jobWaitingOnMe = this;
	SetStatus(JobStatus::WAITING);
}

JobSystem::JobSystem()
	:m_bShouldShutdown(false)
	,m_workerThreads(nullptr)
	,m_numWorkerThreads(0)
	,m_numJobsSubmitted(0)
	,m_numJobsCompleted(0)
{
}

void JobSystem::Init()
{
	m_shutdownJobSystemCs.Init();
	m_numJobsSubmittedCs.Init();
	m_numJobsCompletedCs.Init();

	// Determine how many threads we need to make based on cpu, leaving 1 thread for the main thread
	m_numWorkerThreads = std::thread::hardware_concurrency() - 1;

	// handle case where we get 0 back for thread count
	m_numWorkerThreads = Max(1u, m_numWorkerThreads);

	// allocate Threads
	m_workerThreads = new Thread[m_numWorkerThreads];

	// Start each Thread
	for (U32 i = 0; i < m_numWorkerThreads; ++i)
	{
		m_workerThreads[i].Run(JobWorkerThreadFunc);
	}
}

void JobSystem::Shutdown()
{
	SCOPED_CRITICAL_SECTION(&m_shutdownJobSystemCs);
	m_bShouldShutdown = true;
	// TODO: We need to cleanup s_shutdown_job_system_cs and s_num_jobs_processed_cs
}

void JobSystem::SubmitJob(Job* job)
{
	SM_ASSERT(job != nullptr);

	if (job->IsWaitingOnOthers())
	{
		return;
	}

	job->Acquire();
	job->SetStatus(JobStatus::ENQUEUED);
	m_jobQueue.Push(job);

	m_jobAddedEvent.SignalAndReset();

	SCOPED_CRITICAL_SECTION(&m_numJobsSubmittedCs);
	m_numJobsSubmitted++;
}

Job* JobSystem::SubmitJob(JobFunc func, void* args)
{
	Job* job = new Job();
	job->Init(func, args);
	SubmitJob(job);
	return job;
}

void JobSystem::ReleaseJob(Job* job)
{
	SM_ASSERT(job != nullptr);
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
	Job* job = SubmitJob(func, args);
	ReleaseJob(job);
}

bool JobSystem::IsBusy()
{
	SCOPED_CRITICAL_SECTION(&m_numJobsSubmittedCs);
	SCOPED_CRITICAL_SECTION(&m_numJobsCompletedCs);
	return m_numJobsCompleted < m_numJobsSubmitted;
}

void JobSystem::WaitOnJob(Job* job)
{
	SM_ASSERT(job != nullptr);
	while (job->GetStatus() != JobStatus::FINISHED)
	{
		Thread::Yield();
	}
}

void JobSystem::WaitOnAllJobs()
{
	while (IsBusy())
	{
		Thread::Yield();
	}
}
