#pragma once

#include "Engine/Core/Types.h"

class RenderSettings
{
public:
	RenderSettings();
	void Reset();

	// Debug world grid
	bool m_bDrawDebugWorldGrid;
	F32 m_debugGridFadeDistance;
	F32 m_debugGridMajorLineThickness;
	F32 m_debugGridMinorLineThickness;
};
