
#include "engine/thread/critical_section.h"

critical_section_t critical_section_create()
{
    critical_section_t cs;
	::InitializeCriticalSection(&cs.m_cs);
    return cs;
}

void critical_section_delete(critical_section_t& cs)
{
	::DeleteCriticalSection(&cs.m_cs);
}

void critical_section_lock(critical_section_t& cs)
{
	::EnterCriticalSection(&cs.m_cs);
}

void critical_section_unlock(critical_section_t& cs)
{
	::LeaveCriticalSection(&cs.m_cs);
}

ScopedCriticalSection::ScopedCriticalSection(critical_section_t* cs)
	:m_cs(cs)
{
    critical_section_lock(*m_cs);
}

ScopedCriticalSection::~ScopedCriticalSection()
{
    critical_section_unlock(*m_cs);
}
