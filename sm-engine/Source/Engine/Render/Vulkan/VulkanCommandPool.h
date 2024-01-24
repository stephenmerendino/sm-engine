#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"

class VulkanCommandPool
{
public:
	VulkanCommandPool();

	void Init(const VulkanDevice& device, VkQueueFlags requestedQueueFamilies, VkCommandPoolCreateFlags creatFlags);
	void Destroy(VkDevice device);

	VkCommandPool m_commandPoolHandle;
	VkQueueFlags m_queueFamilies;
	VkCommandPoolCreateFlags m_createFlags;
};
