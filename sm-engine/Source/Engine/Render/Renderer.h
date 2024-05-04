#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Math/IVec2.h"
#include "Engine/Render/RenderSettings.h"

class Camera;
class Window;

class Renderer
{
public:
	virtual void Init(Window* pWindow) = 0;
	virtual void BeginFrame() = 0;
	virtual void Update(F32 ds) = 0;
	virtual void Render() = 0;
	virtual void Shutdown() = 0;
	virtual void SetCamera(const Camera* camera) = 0;
	virtual const Camera* GetCamera() = 0;
	virtual IVec2 GetCurrentResolution() = 0;
	virtual F32 GetAspectRatio() const = 0;
	virtual RenderSettings* GetRenderSettings() = 0;
	virtual const RenderSettings* GetRenderSettings() const = 0;
};

extern Renderer* g_renderer;
