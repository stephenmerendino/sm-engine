#include "engine/thread/crtical_section.h"

critical_section_t create_critical_section()
{
    critical_section_t cs;
	::InitializeCriticalSection(&cs.m_cs);
    return cs;
}

void delete_critical_section(critical_section_t cs)
{
	::DeleteCriticalSection(&cs.m_cs);
}

void lock_cs(critical_section_t cs)
{
	::EnterCriticalSection(&cs.m_cs);
}

void unlock_cs(critical_section_t cs)
{
	::LeaveCriticalSection(&cs.m_cs);
}

ScopedCriticalSection::ScopedCriticalSection(critical_section_t cs)
	:m_cs(cs)
{
    lock_cs(m_cs);
}

ScopedCriticalSection::~ScopedCriticalSection()
{
    unlock_cs(m_cs);
}
