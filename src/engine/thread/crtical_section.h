#pragma once

#include "engine/core/macros.h"
#include "engine/platform/windows_include.h"

struct critical_section_t
{
    CRITICAL_SECTION m_cs; 
};

critical_section_t create_critical_section();
void delete_critical_section(critical_section_t cs);
void lock_cs(critical_section_t cs);
void unlock_cs(critical_section_t cs);

class ScopedCriticalSection 
{
public:
	ScopedCriticalSection(critical_section_t cs);
	~ScopedCriticalSection();

	critical_section_t m_cs;
};

#define SCOPED_CRITICAL_SECTION(cs) ScopedCriticalSection CONCATENATE(scopedCriticalSection, __LINE__)(cs)
