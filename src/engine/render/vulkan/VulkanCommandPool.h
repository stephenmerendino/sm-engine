#pragma once

#include "engine/render/vulkan/VulkanInclude.h"
#include "engine/core/Types.h"

#include <vector>

class VulkanDevice;

class VulkanCommandPool
{
public:
	VulkanCommandPool(VulkanDevice* pDevice);
	~VulkanCommandPool();

	bool Setup(VkQueueFlags queueFamilies, VkCommandPoolCreateFlags createFlags);
	void Teardown();
	VkCommandPool GetHandle();

	std::vector<VkCommandBuffer> AllocateCommandBuffers(VkCommandBufferLevel level, U32 numBuffers);
	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
	VulkanDevice* m_pDevice;
	VkCommandPool m_vkCommandPool;
	VkQueueFlags m_vkQueueFamilies;
	VkCommandPoolCreateFlags m_vkCreateFlags;
};
