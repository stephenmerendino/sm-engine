#include "Engine/Render/Vulkan/VulkanCommandPool.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Common.h"

VulkanCommandPool::VulkanCommandPool(VulkanDevice* pDevice)
	:m_pDevice(pDevice)
	,m_vkCommandPool(VK_NULL_HANDLE)
	,m_vkQueueFamilies(0)
	,m_vkCreateFlags(0)
{
}

VulkanCommandPool::~VulkanCommandPool()
{
}

bool VulkanCommandPool::Setup(VkQueueFlags queueFamilies, VkCommandPoolCreateFlags createFlags)
{
	m_vkQueueFamilies = queueFamilies;
	m_vkCreateFlags = createFlags;
	m_vkCommandPool = m_pDevice->CreateCommandPool(m_vkQueueFamilies, m_vkCreateFlags);
	return true;
}

void VulkanCommandPool::Teardown()
{
	vkDestroyCommandPool(m_pDevice->GetHandle(), m_vkCommandPool, nullptr);
}

VkCommandPool VulkanCommandPool::GetHandle()
{
	return m_vkCommandPool;
}

std::vector<VkCommandBuffer> VulkanCommandPool::AllocateCommandBuffers(VkCommandBufferLevel level, U32 numBuffers)
{
	std::vector<VkCommandBuffer> buffers;
	buffers.resize(numBuffers);

	VkCommandBufferAllocateInfo commandBuffersAllocInfo;
	MemZero(commandBuffersAllocInfo);
	commandBuffersAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBuffersAllocInfo.commandPool = GetHandle();
	commandBuffersAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBuffersAllocInfo.commandBufferCount = (U32)numBuffers;

	VULKAN_ASSERT(vkAllocateCommandBuffers(m_pDevice->GetHandle(), &commandBuffersAllocInfo, buffers.data()));
	return buffers;
}

VkCommandBuffer VulkanCommandPool::BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_vkCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_pDevice->GetHandle(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanCommandPool::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	m_pDevice->FlushGraphics();

	vkFreeCommandBuffers(m_pDevice->GetHandle(), m_vkCommandPool, 1, &commandBuffer);
}