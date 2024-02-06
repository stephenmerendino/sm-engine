#include "Engine/Render/Vulkan/VulkanSync.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"

VulkanSemaphore::VulkanSemaphore()
	:m_semaphoreHandle(VK_NULL_HANDLE)
{
}

void VulkanSemaphore::Init()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	SM_VULKAN_ASSERT(vkCreateSemaphore(VulkanDevice::GetHandle(), &semaphoreCreateInfo, nullptr, &m_semaphoreHandle));
}

void VulkanSemaphore::Destroy()
{
	vkDestroySemaphore(VulkanDevice::GetHandle(), m_semaphoreHandle, nullptr);
}

VulkanFence::VulkanFence()
	:m_fenceHandle(VK_NULL_HANDLE)
{
}

void VulkanFence::Init()
{
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	SM_VULKAN_ASSERT(vkCreateFence(VulkanDevice::GetHandle(), &fenceCreateInfo, nullptr, &m_fenceHandle));
}

void VulkanFence::Reset()
{
	vkResetFences(VulkanDevice::GetHandle(), 1, &m_fenceHandle);
}

void VulkanFence::Wait(U64 timeout)
{
	vkWaitForFences(VulkanDevice::GetHandle(), 1, &m_fenceHandle, VK_TRUE, timeout);
}

void VulkanFence::Destroy()
{
	vkDestroyFence(VulkanDevice::GetHandle(), m_fenceHandle, nullptr);
}