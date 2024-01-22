#pragma once

#include "Engine/Core/Types.h"

namespace Time
{
	void Init();
}

class Stopwatch 
{
public:
	Stopwatch();

	void Start();
	F32 GetElapsedSeconds();
	F32 GetElapsedMs();

	I64 m_startTick;
};
