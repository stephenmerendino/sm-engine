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

	VkCommandBuffer AllocateCommandBuffer(VkCommandBufferLevel level) const;
	void FreeCommandBuffer(VkCommandBuffer commandBuffer) const;

	std::vector<VkCommandBuffer> AllocateCommandBuffers(VkCommandBufferLevel level, U32 numBuffers) const;
	void FreeCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers) const;

	VkCommandBuffer BeginSingleTime() const;
	void EndSingleTime(VkCommandBuffer commandBuffer) const;

	VulkanDevice* m_pDevice;
	VkCommandPool m_commandPoolHandle;
	VkQueueFlags m_queueFamilies;
	VkCommandPoolCreateFlags m_createFlags;
};
