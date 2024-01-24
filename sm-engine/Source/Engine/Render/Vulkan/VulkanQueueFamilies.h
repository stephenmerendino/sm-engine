#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Core/Types.h"

class VulkanQueueFamilies
{
public:
	VulkanQueueFamilies();

	void Init(VkPhysicalDevice device, VkSurfaceKHR surface);
	bool HasRequiredFamilies();

	I32 m_graphicsFamily;
	I32 m_presentFamily;

	enum
	{
		kInvalidFamilyIndex = -1
	};
};
