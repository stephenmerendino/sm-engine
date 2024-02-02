#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"

class VulkanDevice;

class VulkanSemaphore
{
public:
	VulkanSemaphore();

	void Init(const VulkanDevice* pDevice);
	void Destroy();

	const VulkanDevice* m_pDevice;
	VkSemaphore m_semaphoreHandle;
};

class VulkanFence
{
public:
	VulkanFence();

	void Init(const VulkanDevice* pDevice);
	void Reset();
	void Wait(U64 timeout = UINT64_MAX);
	void Destroy();

	const VulkanDevice* m_pDevice;
	VkFence m_fenceHandle;
};
