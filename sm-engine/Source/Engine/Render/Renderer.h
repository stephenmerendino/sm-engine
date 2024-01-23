#pragma once

class Window;

class Renderer
{
public:
	virtual void Init(Window* pWindow) = 0;
	virtual void Shutdown() = 0;
};

extern Renderer* g_renderer;
