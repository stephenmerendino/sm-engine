#pragma once

#include "Engine/Render/Renderer.h"

class VulkanRenderer : public Renderer
{
public:
	virtual void Init(Window* pWindow) final;
	virtual void Shutdown() final;
};
