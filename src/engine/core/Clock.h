#pragma once

#include "engine/core/Common.h"

class Clock
{
public:
	Clock();
	~Clock();

	void Start();
	F32 GetSecondsElapsed();
	F32 GetMsElapsed();

	static bool Init();

private:
	I64 m_startTick;
};
