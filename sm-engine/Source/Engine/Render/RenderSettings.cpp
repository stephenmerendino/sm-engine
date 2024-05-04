#include "Engine/Render/RenderSettings.h"

RenderSettings::RenderSettings()
{
	Reset();
}

void RenderSettings::Reset()
{
	m_bDrawDebugWorldGrid = true;
}