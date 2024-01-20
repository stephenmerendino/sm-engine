#include "Engine/Job/JobSystem.h"

JobSystem g_jobSystem;

bool JobSystem::Init()
{
	return true;
}

void JobSystem::Shutdown()
{

}

//#include "engine/job/job_system.h"
//#include "engine/core/assert.h"
//#include "engine/thread/critical_section.h"
//#include "engine/thread/event.h"
//#include "engine/thread/thread.h"
//#include "engine/thread/thread_safe_queue.h"
//#include "engine/math/math_utils.h"
//
//static event_t                  s_jobs_added_event;
//static ThreadSafeQueue<job_t*>  s_job_queue;
//static i64                      s_num_jobs_submitted = 0;
//static i64                      s_num_jobs_completed = 0;
//static bool                     s_should_shutdown_job_system = false;
//static critical_section_t       s_shutdown_job_system_cs;
//static critical_section_t       s_num_jobs_processed_cs;
//static u32                      s_num_worker_threads = 0;
//static thread_t* s_worker_threads = nullptr;
//
//static
//void job_setup(job_t* job, job_func_t func, void* args)
//{
//	SM_ASSERT(nullptr != job);
//	job->m_func = func;
//	job->m_args = args;
//	job->m_job_waiting_on_me = nullptr;
//	job->m_waiting_on_count = 0;
//	job->m_status = JobStatus::CREATED;
//	job->m_job_status_rw_cs = critical_section_create();
//	job->m_ref_count = 1;
//}
//
//static
//void job_acquire(job_t* job)
//{
//	SM_ASSERT(nullptr != job);
//	atomic_incr(&job->m_ref_count);
//}
//
//static
//void job_release(job_t* job)
//{
//	SM_ASSERT(nullptr != job);
//	atomic_decr(&job->m_ref_count);
//}
//
//static
//bool job_is_released(job_t* job)
//{
//	SM_ASSERT(nullptr != job);
//	return job->m_ref_count == 0;
//}
//
//static
//void job_set_status(job_t* job, JobStatus status)
//{
//	SM_ASSERT(nullptr != job);
//	SCOPED_CRITICAL_SECTION(&job->m_job_status_rw_cs);
//	job->m_status = status;
//}
//
//static
//void job_notify_waiting_on_finished(job_t* job)
//{
//	SM_ASSERT(nullptr != job);
//	job->m_waiting_on_count--;
//	job_system_submit_job(job);
//}
//
//static
//void job_execute(job_t* job)
//{
//	SM_ASSERT(nullptr != job);
//	job_set_status(job, JobStatus::RUNNING);
//	job->m_func(job->m_args);
//}
//
//static
//void job_finish(job_t* job)
//{
//	SM_ASSERT(nullptr != job);
//	job_set_status(job, JobStatus::FINISHED);
//	if (job->m_job_waiting_on_me)
//	{
//		job_notify_waiting_on_finished(job->m_job_waiting_on_me);
//	}
//}
//
//static
//bool job_is_waiting_on_others(job_t* job)
//{
//	SM_ASSERT(nullptr != job);
//	return job_get_status(job) == JobStatus::WAITING;
//}
//
//void job_wait_on(job_t* job, job_t* job_to_wait_for)
//{
//	job->m_waiting_on_count++;
//	job_to_wait_for->m_job_waiting_on_me = job;
//	job_set_status(job, JobStatus::WAITING);
//}
//
//JobStatus job_get_status(job_t* job)
//{
//	SCOPED_CRITICAL_SECTION(&job->m_job_status_rw_cs);
//	return job->m_status;
//}
//
//// this function needs to pull jobs out of the job queue and execute them
//static
//DWORD WINAPI job_worker_thread_func(void* args)
//{
//	UNUSED(args);
//
//	while (true)
//	{
//		{
//			SCOPED_CRITICAL_SECTION(&s_shutdown_job_system_cs);
//			if (s_should_shutdown_job_system && s_job_queue.empty())
//			{
//				break;
//			}
//		}
//
//		job_t* job = nullptr;
//		if (s_job_queue.pop(&job))
//		{
//			job_execute(job);
//			job_finish(job);
//			job_system_release_job(job);
//
//			SCOPED_CRITICAL_SECTION(&s_num_jobs_processed_cs);
//			s_num_jobs_completed++;
//		}
//		else
//		{
//			event_wait(s_jobs_added_event);
//		}
//	}
//
//	return 0;
//}
//
//void job_system_init()
//{
//	s_shutdown_job_system_cs = critical_section_create();
//	s_num_jobs_processed_cs = critical_section_create();
//
//	// determine how many threads we need to make based on cpu, leaving 1 thread for the main thread
//	s_num_worker_threads = std::thread::hardware_concurrency() - 1;
//
//	// handle case where we get 0 back for thread count
//	s_num_worker_threads = max(1u, s_num_worker_threads);
//
//	// allocate Threads
//	s_worker_threads = new thread_t[s_num_worker_threads];
//
//	// Start each Thread
//	for (u32 i = 0; i < s_num_worker_threads; ++i)
//	{
//		s_worker_threads[i] = create_thread(job_worker_thread_func);
//		thread_t& t = s_worker_threads[i];
//		thread_run(t);
//	}
//}
//
//void job_system_shutdown()
//{
//	SCOPED_CRITICAL_SECTION(&s_shutdown_job_system_cs);
//	s_should_shutdown_job_system = true;
//
//	// TODO(smerendino): We need to cleanup s_shutdown_job_system_cs and s_num_jobs_processed_cs
//}
//
//job_t* job_system_create_job(job_func_t func, void* args)
//{
//	job_t* j = new job_t;
//	job_setup(j, func, args);
//	return j;
//}
//
//void job_system_submit_job(job_t* job)
//{
//	SM_ASSERT(job);
//
//	job_acquire(job);
//
//	if (job_is_waiting_on_others(job))
//	{
//		return;
//	}
//
//	job_set_status(job, JobStatus::ENQUEUED);
//
//	s_job_queue.push(job);
//	event_signal_and_reset(s_jobs_added_event);
//
//	SCOPED_CRITICAL_SECTION(&s_num_jobs_processed_cs);
//	s_num_jobs_submitted++;
//}
//
//void job_system_release_job(job_t* job)
//{
//	SM_ASSERT(job);
//
//	job_release(job);
//	if (job_is_released(job))
//	{
//		delete job;
//	}
//}
//
//void job_system_submit_and_release_job(job_t* job)
//{
//	job_system_submit_job(job);
//	job_system_release_job(job);
//}
//
//void job_system_submit_and_release_job(job_func_t func, void* args)
//{
//	job_t* new_job = job_system_create_job(func, args);
//	job_system_submit_and_release_job(new_job);
//}
//
//bool job_system_is_busy()
//{
//	SCOPED_CRITICAL_SECTION(&s_num_jobs_processed_cs);
//	return s_num_jobs_completed < s_num_jobs_submitted;
//}
//
//void job_system_wait_on_job(job_t* job)
//{
//	while (job_get_status(job) != JobStatus::FINISHED)
//	{
//		thread_yield();
//	}
//}
//
//void job_system_wait_all()
//{
//	while (job_system_is_busy())
//	{
//		thread_yield();
//	}
//}
