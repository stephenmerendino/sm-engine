#include "Engine/Render/Vulkan/VulkanCommandPool.h"
#include "Engine/Render/Vulkan/VulkanCommands.h"
#include "Engine/Core/Assert_old.h"

VulkanCommandPool::VulkanCommandPool()
	:m_commandPoolHandle(VK_NULL_HANDLE)
	,m_queueFamilies(0)
	,m_createFlags(0)
{
}

void VulkanCommandPool::Init(VkQueueFlags requestedQueueFamilies, VkCommandPoolCreateFlags createFlags)
{
	VulkanQueueFamilies queueFamilies = VulkanDevice::Get()->m_queueFamilies;

	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = createFlags;

	// Graphics + Compute requested (normal queue)
	if (requestedQueueFamilies & VK_QUEUE_GRAPHICS_BIT)
	{
		createInfo.queueFamilyIndex = VulkanDevice::Get()->m_queueFamilies.m_graphicsAndComputeFamilyIndex;
	}

	// Only Compute requested (async compute)
	if ((requestedQueueFamilies & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT)) == VK_QUEUE_COMPUTE_BIT)
	{
		createInfo.queueFamilyIndex = VulkanDevice::Get()->m_queueFamilies.m_asyncComputeFamilyIndex;
	}

	SM_VULKAN_ASSERT_OLD(vkCreateCommandPool(VulkanDevice::GetHandle(), &createInfo, nullptr, &m_commandPoolHandle));
}

void VulkanCommandPool::Destroy()
{
	vkDestroyCommandPool(VulkanDevice::GetHandle(), m_commandPoolHandle, nullptr);
}

VkCommandBuffer VulkanCommandPool::AllocateCommandBuffer(VkCommandBufferLevel level) const
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = level;
	allocInfo.commandPool = m_commandPoolHandle;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(VulkanDevice::GetHandle(), &allocInfo, &commandBuffer);

	return commandBuffer;
}

void VulkanCommandPool::FreeCommandBuffer(VkCommandBuffer commandBuffer) const
{
	vkFreeCommandBuffers(VulkanDevice::GetHandle(), m_commandPoolHandle, 1, &commandBuffer);
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

	SM_VULKAN_ASSERT_OLD(vkAllocateCommandBuffers(VulkanDevice::GetHandle(), &allocInfo, buffers.data()));
	return buffers;
}

void VulkanCommandPool::FreeCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers) const
{
	vkFreeCommandBuffers(VulkanDevice::GetHandle(), m_commandPoolHandle, (U32)commandBuffers.size(), commandBuffers.data());
}

VkCommandBuffer VulkanCommandPool::BeginSingleTime() const
{
	VkCommandBuffer commandBuffer = AllocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VulkanCommands::Begin(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	return commandBuffer;
}

void VulkanCommandPool::EndAndSubmitSingleTime(VkCommandBuffer commandBuffer) const
{
	VulkanCommands::End(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(VulkanDevice::Get()->m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(VulkanDevice::Get()->m_graphicsQueue);

	FreeCommandBuffer(commandBuffer);
}