#include "Engine/Job/Job.h"
#include "Engine/Job/JobSystem.h"
#include "Engine/Core/Assert.h"

Job::Job(JobFunc jobFunc, void* jobArgs)
	:m_jobFunc(jobFunc)
	, m_jobArgs(jobArgs)
	, m_pParent(nullptr)
	, m_childrenCount(0)
	, m_status(JobStatus::CREATED)
	, m_refCount(1)
{
}

Job::~Job()
{
	delete m_jobArgs;
}

void Job::Acquire()
{
	m_refCount++;
}

void Job::Release()
{
	m_refCount--;
}

bool Job::IsReleased() const
{
	return m_refCount.GetValue() == 0;
}

void Job::Execute()
{
	SetJobStatus(JobStatus::RUNNING);
	m_jobFunc(m_jobArgs);
}

void Job::DependsOn(Job* dependency)
{
	ASSERT(dependency);
	ASSERT(m_pParent == nullptr);

	dependency->m_pParent = this;
	m_childrenCount++;

	SetJobStatus(JobStatus::WAITING_ON_CHILDREN);
}

void Job::Finish()
{
	SetJobStatus(JobStatus::FINISHED);
	if (m_pParent)
	{
		m_pParent->DependencyFinished();
	}
}

void Job::DependencyFinished()
{
	m_childrenCount--;
	g_jobSystem.SubmitJob(this);
}

bool Job::IsWaitingOnDependencies()
{
	return GetJobStatus() == JobStatus::WAITING_ON_CHILDREN;
}

JobStatus Job::GetJobStatus()
{
	SCOPED_CRITICAL_SECTION(&m_jobStatusRwGuard);
	return m_status;
}

void Job::SetJobStatus(JobStatus status)
{
	SCOPED_CRITICAL_SECTION(&m_jobStatusRwGuard);
	m_status = status;
}