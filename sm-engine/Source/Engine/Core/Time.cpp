#include "Engine/Core/Time.h"
#include "Engine/Platform/WindowsInclude.h"
#include "Engine/Core/Assert.h"

static I64 s_freqPerSec;

static I64 GetCurrentTick()
{
	LARGE_INTEGER tick;
	SM_ASSERT(QueryPerformanceCounter(&tick));
	return tick.QuadPart;
}

void TimeInit()
{
	static bool didInit = false;
	if (!didInit)
	{
		LARGE_INTEGER freq;
		BOOL res = QueryPerformanceFrequency(&freq);
		SM_ASSERT(res);
		s_freqPerSec = freq.QuadPart;
		didInit = true;
	}
}

Stopwatch::Stopwatch()
	:m_startTick(0)
{
}

void Stopwatch::Start()
{
	m_startTick = GetCurrentTick();
}

F32 Stopwatch::GetElapsedSeconds()
{
	I64 curTick = GetCurrentTick();
	I64 ticksSinceStart = curTick - m_startTick;
	F32 seconds_elapsed = (F32)(ticksSinceStart) / (F32)(s_freqPerSec);
	return seconds_elapsed;
}

F32 Stopwatch::GetElapsedMs()
{
	return GetElapsedSeconds() * 1000.0f;
}