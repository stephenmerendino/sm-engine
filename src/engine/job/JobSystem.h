#pragma once

#include "engine/job/Job.h"
#include "engine/thread/Thread.h"

class JobSystem
{
public:
	JobSystem();
	~JobSystem();

	bool Init();
	void Shutdown();

	Job* CreateJob(JobFunc func, void* args =  nullptr);
	void SubmitJob(Job* job);
	void ReleaseJob(Job* job);
	void SubmitAndReleaseJob(Job* job);
	void SubmitAndReleaseJob(JobFunc func, void* args = nullptr);

	bool IsBusy();
	void WakeUpWorkers();

	void WaitAll();
	void WaitOnJob(Job* job);

private:
	U32 m_numWorkersThreads;
	Thread* m_workerThreads;
};

extern JobSystem g_jobSystem;
