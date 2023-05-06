#pragma once

#include "engine/core/StringUtils.h"
#include "engine/platform/WindowsInclude.h"

class CriticalSection
{
public:
	CriticalSection();
	~CriticalSection();

	void Lock();
	void Unlock();

private:
	CRITICAL_SECTION m_criticalSection;
};

class ScopedCriticalSection
{
public:
	ScopedCriticalSection(CriticalSection* criticalSection);
	~ScopedCriticalSection();

private:
	CriticalSection* m_criticalSection;
};

#define SCOPED_CRITICAL_SECTION(criticalSection) ScopedCriticalSection CONCATENATE(scopedCriticalSection, __LINE__)(criticalSection)
