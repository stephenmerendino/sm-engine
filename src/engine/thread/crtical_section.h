#pragma once

#include "engine/core/macros.h"
#include "engine/platform/windows_include.h"

struct critical_section_t
{
    CRITICAL_SECTION m_cs; 
};

critical_section_t critical_section_create();
void critical_section_delete(critical_section_t cs);
void critical_section_lock(critical_section_t cs);
void critical_section_unlock(critical_section_t cs);

class ScopedCriticalSection 
{
public:
	ScopedCriticalSection(critical_section_t cs);
	~ScopedCriticalSection();

	critical_section_t m_cs;
};

#define SCOPED_CRITICAL_SECTION(cs) ScopedCriticalSection CONCATENATE(scopedCriticalSection, __LINE__)(cs)
