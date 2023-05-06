#include "engine/core/Clock.h"
#include "engine/platform/WindowsInclude.h"

static I64 s_freqPerSecond;

static I64 GetCurrentTick()
{
	LARGE_INTEGER tick;
	ASSERT(QueryPerformanceCounter(&tick));
	return tick.QuadPart;
}

Clock::Clock()
	:m_startTick(0)
{
}

Clock::~Clock()
{
}

void Clock::Start()
{
	m_startTick = GetCurrentTick();
}

F32 Clock::GetSecondsElapsed() 
{
	I64 curTick = GetCurrentTick();
	I64 ticksSinceStart = curTick - m_startTick;
	F32 secondsElapsed = static_cast<F32>(ticksSinceStart) / static_cast<F32>(s_freqPerSecond);
	return secondsElapsed;
}

F32 Clock::GetMsElapsed()
{
	return GetSecondsElapsed() * 1000.0f;
}

bool Clock::Init()
{
	static bool alreadyInit = false;
	if (!alreadyInit)
	{
		LARGE_INTEGER freq;
		BOOL res = QueryPerformanceFrequency(&freq);
		ASSERT(res);
		s_freqPerSecond = freq.QuadPart;
		alreadyInit = true;
	}
	return true;
}
