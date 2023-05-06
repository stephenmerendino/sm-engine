#pragma once

#include "engine/thread/CriticalSection.h"

#include <queue>

template<typename T>
class ThreadSafeQueue
{
public:
	void Push(T& t);
	bool Pop(T* outT);
	T Front();
	bool Empty();

private:
	std::queue<T> m_queue;
	CriticalSection m_readWriteLock;
};

template<typename T>
void ThreadSafeQueue<T>::Push(T& t)
{
	SCOPED_CRITICAL_SECTION(&m_readWriteLock);
	m_queue.push(t);
}

template<typename T>
bool ThreadSafeQueue<T>::Pop(T* outT)
{
	SCOPED_CRITICAL_SECTION(&m_readWriteLock);

	if (m_queue.empty())
	{
		return false;
	}

	*outT = m_queue.front();
	m_queue.pop();
	return true;
}

template<typename T>
T ThreadSafeQueue<T>::Front()
{
	SCOPED_CRITICAL_SECTION(&m_readWriteLock);
	return m_queue.front();
}

template<typename T>
bool ThreadSafeQueue<T>::Empty()
{
	SCOPED_CRITICAL_SECTION(&m_readWriteLock);
	return m_queue.empty();
}
