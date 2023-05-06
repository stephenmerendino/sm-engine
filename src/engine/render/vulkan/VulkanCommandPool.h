#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Core/Types.h"

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