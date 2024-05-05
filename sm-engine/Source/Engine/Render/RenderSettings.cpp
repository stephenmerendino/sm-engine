#include "Engine/Render/RenderSettings.h"

RenderSettings::RenderSettings()
{
	Reset();
}

void RenderSettings::Reset()
{
	m_bDrawDebugWorldGrid = true;
	m_debugGridFadeDistance = 0.9985f;
	m_debugGridMajorLineThickness = 0.005f;
	m_debugGridMinorLineThickness = 0.003f;
}