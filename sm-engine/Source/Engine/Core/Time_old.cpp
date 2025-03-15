#include "Engine/Core/Time_old.h"
#include "Engine/Platform/WindowsInclude.h"
#include "Engine/Core/Assert_old.h"

static I64 s_freqPerSec;

static I64 GetCurrentTick()
{
	LARGE_INTEGER tick;
	SM_ASSERT_OLD(QueryPerformanceCounter(&tick));
	return tick.QuadPart;
}

void Time::Init()
{
	static bool didInit = false;
	if (!didInit)
	{
		LARGE_INTEGER freq;
		BOOL res = QueryPerformanceFrequency(&freq);
		SM_ASSERT_OLD(res);
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
	F32 secondsElapsed = (F32)(ticksSinceStart) / (F32)(s_freqPerSec);
	return secondsElapsed;
}

F32 Stopwatch::GetElapsedMs()
{
	return GetElapsedSeconds() * 1000.0f;
}