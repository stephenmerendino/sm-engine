#include "Engine/Thread/CriticalSection.h"

CriticalSection::CriticalSection()
{
	::InitializeCriticalSection(&m_criticalSection);
}

CriticalSection::~CriticalSection()
{
	::DeleteCriticalSection(&m_criticalSection);
}

void CriticalSection::Lock()
{
	::EnterCriticalSection(&m_criticalSection);
}

void CriticalSection::Unlock()
{
	::LeaveCriticalSection(&m_criticalSection);
}

ScopedCriticalSection::ScopedCriticalSection(CriticalSection* criticalSection)
	:m_criticalSection(criticalSection)
{
	m_criticalSection->Lock();
}

ScopedCriticalSection::~ScopedCriticalSection()
{
	m_criticalSection->Unlock();
}