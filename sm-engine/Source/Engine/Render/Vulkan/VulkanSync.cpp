#include "Engine/Render/Vulkan/VulkanSync.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"

VulkanSemaphore::VulkanSemaphore()
	:m_pDevice(nullptr)
	,m_semaphoreHandle(VK_NULL_HANDLE)
{
}

void VulkanSemaphore::Init(const VulkanDevice* pDevice)
{
	m_pDevice = pDevice;

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	SM_VULKAN_ASSERT(vkCreateSemaphore(m_pDevice->m_deviceHandle, &semaphoreCreateInfo, nullptr, &m_semaphoreHandle));
}

void VulkanSemaphore::Destroy()
{
	vkDestroySemaphore(m_pDevice->m_deviceHandle, m_semaphoreHandle, nullptr);
}

VulkanFence::VulkanFence()
	:m_pDevice(nullptr)
	,m_fenceHandle(VK_NULL_HANDLE)
{
}

void VulkanFence::Init(const VulkanDevice* pDevice)
{
	m_pDevice = pDevice;

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	SM_VULKAN_ASSERT(vkCreateFence(m_pDevice->m_deviceHandle, &fenceCreateInfo, nullptr, &m_fenceHandle));
}

void VulkanFence::Reset()
{
	vkResetFences(m_pDevice->m_deviceHandle, 1, &m_fenceHandle);
}

void VulkanFence::Wait(U64 timeout)
{
	vkWaitForFences(m_pDevice->m_deviceHandle, 1, &m_fenceHandle, VK_TRUE, timeout);
}

void VulkanFence::Destroy()
{
	vkDestroyFence(m_pDevice->m_deviceHandle, m_fenceHandle, nullptr);
}