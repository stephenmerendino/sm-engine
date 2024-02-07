#pragma once

class Camera;
class Window;

class Renderer
{
public:
	virtual void Init(Window* pWindow) = 0;
	virtual void RenderFrame() = 0;
	virtual void Shutdown() = 0;
	virtual void SetCamera(const Camera* camera) = 0;
};

extern Renderer* g_renderer;
