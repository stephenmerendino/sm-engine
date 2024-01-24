#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Core/Types.h"

class VulkanQueueFamilies
{
public:
	VulkanQueueFamilies();

	void Init(VkPhysicalDevice device, VkSurfaceKHR surface);
	bool HasRequiredFamilies();

	I32 m_graphicsFamilyIndex;
	I32 m_presentFamilyIndex;

	enum
	{
		kInvalidFamilyIndex = -1
	};
};
