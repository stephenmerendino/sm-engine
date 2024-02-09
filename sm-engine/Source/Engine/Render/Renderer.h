#pragma once

#include "Engine/Core/Types.h"

class Camera;
class Window;

class Renderer
{
public:
	virtual void Init(Window* pWindow) = 0;
	virtual void Update(F32 ds) = 0;
	virtual void Render() = 0;
	virtual void Shutdown() = 0;
	virtual void SetCamera(const Camera* camera) = 0;
};

extern Renderer* g_renderer;
