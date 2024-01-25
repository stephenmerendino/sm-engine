#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"

class VulkanDevice;

class VulkanCommandPool
{
public:
	VulkanCommandPool();

	void Init(VulkanDevice* pDevice, VkQueueFlags requestedQueueFamilies, VkCommandPoolCreateFlags creatFlags);
	void Destroy();

	//VkCommandBuffer AllocateCommandBuffer(context_t& context, VkCommandBufferLevel level, u32 num_buffers);
	//void FreeCommandBuffer(context_t& context, VkCommandBuffer commandBufferr);
	//std::vector<VkCommandBuffer> AllocateCommandBuffers(context_t& context, VkCommandBufferLevel level, u32 num_buffers);
	//void FreeCommandBuffers(context_t& context, std::vector<VkCommandBuffer>& command_buffers);

	//void command_buffer_begin(VkCommandBuffer command_buffer)
	//void command_buffer_end(VkCommandBuffer command_buffer)

	VkCommandBuffer BeginSingleTime();
	void EndSingleTime(VkCommandBuffer commandBuffer);

	VulkanDevice* m_pDevice;
	VkCommandPool m_commandPoolHandle;
	VkQueueFlags m_queueFamilies;
	VkCommandPoolCreateFlags m_createFlags;
};
