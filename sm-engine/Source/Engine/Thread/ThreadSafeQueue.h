#pragma once

#include "Engine/Thread/CriticalSection.h"
#include <queue>

template<typename T>
class ThreadSafeQueue
{
public:
	void Init();
	void Destroy();

	void Push(T& t);
	bool Pop(T* outT);
	T Front();
	bool Empty();

	std::queue<T> m_queue;
	CriticalSection m_rwLock;
};

template<typename T>
void ThreadSafeQueue<T>::Init()
{
	m_rwLock.Init();
}

template<typename T>
void ThreadSafeQueue<T>::Destroy()
{
	m_rwLock.Destroy();
}

template<typename T>
void ThreadSafeQueue<T>::Push(T& t)
{
	SCOPED_CRITICAL_SECTION(&m_rwLock);
	m_queue.push(t);
}

template<typename T>
bool ThreadSafeQueue<T>::Pop(T* outT)
{
	SCOPED_CRITICAL_SECTION(&m_rwLock);

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
	SCOPED_CRITICAL_SECTION(&m_rwLock);
	return m_queue.front();
}

template<typename T>
bool ThreadSafeQueue<T>::Empty()
{
	SCOPED_CRITICAL_SECTION(&m_rwLock);
	return m_queue.empty();
}
