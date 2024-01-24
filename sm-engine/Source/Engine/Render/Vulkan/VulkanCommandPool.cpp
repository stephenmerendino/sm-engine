#include "Engine/Render/Vulkan/VulkanCommandPool.h"
#include "Engine/Core/Assert.h"

VulkanCommandPool::VulkanCommandPool()
	:m_commandPoolHandle(VK_NULL_HANDLE)
	,m_queueFamilies(0)
	,m_createFlags(0)
{
}

void VulkanCommandPool::Init(const VulkanDevice& device, VkQueueFlags requestedQueueFamilies, VkCommandPoolCreateFlags createFlags)
{
	VulkanQueueFamilies queueFamilies = device.m_queueFamilies;

	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = createFlags;

	if (requestedQueueFamilies & VK_QUEUE_GRAPHICS_BIT)
	{
		createInfo.queueFamilyIndex = device.m_queueFamilies.m_graphicsFamilyIndex;
	}

	SM_VULKAN_ASSERT(vkCreateCommandPool(device.m_deviceHandle, &createInfo, nullptr, &m_commandPoolHandle));
}

void VulkanCommandPool::Destroy(VkDevice device)
{
	vkDestroyCommandPool(device, m_commandPoolHandle, nullptr);
}