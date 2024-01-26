#include "Engine/Render/Vulkan/VulkanCommandPool.h"
#include "Engine/Render/Vulkan/VulkanCommands.h"
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

VkCommandBuffer VulkanCommandPool::AllocateCommandBuffer(VkCommandBufferLevel level) const
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = level;
	allocInfo.commandPool = m_commandPoolHandle;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_pDevice->m_deviceHandle, &allocInfo, &commandBuffer);

	return commandBuffer;
}

void VulkanCommandPool::FreeCommandBuffer(VkCommandBuffer commandBuffer) const
{
	vkFreeCommandBuffers(m_pDevice->m_deviceHandle, m_commandPoolHandle, 1, &commandBuffer);
}

std::vector<VkCommandBuffer> VulkanCommandPool::AllocateCommandBuffers(VkCommandBufferLevel level, U32 numBuffers) const
{
	std::vector<VkCommandBuffer> buffers;
	buffers.resize(numBuffers);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPoolHandle;
	allocInfo.level = level;
	allocInfo.commandBufferCount = numBuffers;

	SM_VULKAN_ASSERT(vkAllocateCommandBuffers(m_pDevice->m_deviceHandle, &allocInfo, buffers.data()));
	return buffers;
}

void VulkanCommandPool::FreeCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers) const
{
	vkFreeCommandBuffers(m_pDevice->m_deviceHandle, m_commandPoolHandle, (U32)commandBuffers.size(), commandBuffers.data());
}

VkCommandBuffer VulkanCommandPool::BeginSingleTime() const
{
	VkCommandBuffer commandBuffer = AllocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VulkanCommands::Begin(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	return commandBuffer;
}

void VulkanCommandPool::EndSingleTime(VkCommandBuffer commandBuffer) const
{
	VulkanCommands::End(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_pDevice->m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_pDevice->m_graphicsQueue);

	FreeCommandBuffer(commandBuffer);
}