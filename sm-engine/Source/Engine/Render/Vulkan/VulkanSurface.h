#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"

class Window;

class VulkanSurface
{
public:
	VulkanSurface();

	void Init(Window* pWindow, VkInstance instance);
	void Destroy(VkInstance instance);

	VkSurfaceKHR m_surfaceHandle;
};
