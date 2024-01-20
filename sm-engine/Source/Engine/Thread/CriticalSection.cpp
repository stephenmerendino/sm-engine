#include "Engine/Thread/CriticalSection.h"

CriticalSection::CriticalSection()
{
	::InitializeCriticalSection(&m_winCs);
}

CriticalSection::~CriticalSection()
{
	::DeleteCriticalSection(&m_winCs);
}

void CriticalSection::Lock()
{
	::EnterCriticalSection(&m_winCs);
}

void CriticalSection::Unlock()
{
	::LeaveCriticalSection(&m_winCs);
}

ScopedCriticalSection::ScopedCriticalSection(CriticalSection* cs)
	:m_cs(cs)
{
	m_cs->Lock();
}

ScopedCriticalSection::~ScopedCriticalSection()
{
	m_cs->Unlock();
}
