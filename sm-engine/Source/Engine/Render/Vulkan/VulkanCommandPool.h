#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include <vector>

class VulkanDevice;

class VulkanCommandPool
{
public:
	VulkanCommandPool();

	void Init(VulkanDevice* pDevice, VkQueueFlags requestedQueueFamilies, VkCommandPoolCreateFlags creatFlags);
	void Destroy();

	VkCommandBuffer AllocateCommandBuffer(VkCommandBufferLevel level);
	void FreeCommandBuffer(VkCommandBuffer commandBuffer);

	std::vector<VkCommandBuffer> AllocateCommandBuffers(VkCommandBufferLevel level, U32 numBuffers);
	void FreeCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers);

	VkCommandBuffer BeginSingleTime();
	void EndSingleTime(VkCommandBuffer commandBuffer);

	VulkanDevice* m_pDevice;
	VkCommandPool m_commandPoolHandle;
	VkQueueFlags m_queueFamilies;
	VkCommandPoolCreateFlags m_createFlags;
};
