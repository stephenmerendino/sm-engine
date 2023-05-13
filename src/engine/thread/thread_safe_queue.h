#pragma once

#include "engine/thread/crtical_section.h"

#include <queue>

template<typename T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue();
    ~ThreadSafeQueue();

	void push(T& t);
	bool pop(T* outT);
	T front();
	bool empty();

	std::queue<T> m_queue;
	critical_section_t m_rw_lock;
};

template<typename T>
ThreadSafeQueue<T>::ThreadSafeQueue()
{
    m_rw_lock = critical_section_create();
}

template<typename T>
ThreadSafeQueue<T>::~ThreadSafeQueue()
{
    critical_section_delete(m_rw_lock);
}

template<typename T>
void ThreadSafeQueue<T>::push(T& t)
{
	SCOPED_CRITICAL_SECTION(&m_rw_lock);
	m_queue.push(t);
}

template<typename T>
bool ThreadSafeQueue<T>::pop(T* out_t)
{
	SCOPED_CRITICAL_SECTION(&m_rw_lock);

	if (m_queue.empty())
	{
		return false;
	}

	*out_t = m_queue.front();
	m_queue.pop();
	return true;
}

template<typename T>
T ThreadSafeQueue<T>::front()
{
	SCOPED_CRITICAL_SECTION(&m_rw_lock);
	return m_queue.front();
}

template<typename T>
bool ThreadSafeQueue<T>::empty()
{
	SCOPED_CRITICAL_SECTION(&m_rw_lock);
	return m_queue.empty();
}
