#pragma once

#include "engine/render/Window.h"
#include "engine/render/vulkan/VulkanInclude.h"

class VulkanSurface
{
public:
	VulkanSurface(const Window* pWindow);
	~VulkanSurface();

	bool Setup();
	void Teardown();
	U32 GetWindowWidth() const;
	U32 GetWindowHeight() const;

	VkSurfaceKHR GetHandle();

private:
	const Window* m_pWindow;
	VkSurfaceKHR m_vkSurface;
};
