#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"

class VulkanDevice;

class VulkanSemaphore
{
public:
	VulkanSemaphore();

	void Init();
	void Destroy();

	VkSemaphore m_semaphoreHandle;
};

class VulkanFence
{
public:
	VulkanFence();

	void Init();
	void Reset();
	void Wait(U64 timeout = UINT64_MAX);
	void WaitAndReset(U64 timeout = UINT64_MAX);
	void Destroy();

	VkFence m_fenceHandle;
};
