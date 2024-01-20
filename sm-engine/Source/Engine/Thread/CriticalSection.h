#pragma once

#include "Engine/Core/Macros.h"
#include "Engine/Platform/WindowsInclude.h"

class CriticalSection
{
public:
	CriticalSection();
	~CriticalSection();

	void Lock();
	void Unlock();

	CRITICAL_SECTION m_winCs;
};

class ScopedCriticalSection
{
public:
	ScopedCriticalSection(CriticalSection* cs);
	~ScopedCriticalSection();

	CriticalSection* m_cs;
};

#define SCOPED_CRITICAL_SECTION(cs) ScopedCriticalSection CONCATENATE(scopedCriticalSection, __LINE__)(cs)
