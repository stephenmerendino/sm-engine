#include "Engine/Render/Vulkan/VulkanCommandPool.h"
#include "Engine/Core/Assert.h"

VulkanCommandPool::VulkanCommandPool()
	:m_pDevice(nullptr)
	,m_commandPoolHandle(VK_NULL_HANDLE)
	,m_queueFamilies(0)
	,m_createFlags(0)
{
}

void VulkanCommandPool::Init(VulkanDevice* pDevice, VkQueueFlags requestedQueueFamilies, VkCommandPoolCreateFlags createFlags)
{
	SM_ASSERT(pDevice != nullptr);
	m_pDevice = pDevice;

	VulkanQueueFamilies queueFamilies = m_pDevice->m_queueFamilies;

	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = createFlags;

	if (requestedQueueFamilies & VK_QUEUE_GRAPHICS_BIT)
	{
		createInfo.queueFamilyIndex = m_pDevice->m_queueFamilies.m_graphicsFamilyIndex;
	}

	SM_VULKAN_ASSERT(vkCreateCommandPool(m_pDevice->m_deviceHandle, &createInfo, nullptr, &m_commandPoolHandle));
}

void VulkanCommandPool::Destroy()
{
	vkDestroyCommandPool(m_pDevice->m_deviceHandle, m_commandPoolHandle, nullptr);
}

VkCommandBuffer VulkanCommandPool::BeginSingleTime()
{
	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = m_commandPoolHandle;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(m_pDevice->m_deviceHandle, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}

void VulkanCommandPool::EndSingleTime(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_pDevice->m_graphicsQueue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_pDevice->m_graphicsQueue);

	vkFreeCommandBuffers(m_pDevice->m_deviceHandle, m_commandPoolHandle, 1, &commandBuffer);
}