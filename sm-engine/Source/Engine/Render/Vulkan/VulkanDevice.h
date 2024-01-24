#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanQueueFamilies.h"

class VulkanDevice
{
public:
	VulkanDevice();

	void Init(VkInstance instance, VkSurfaceKHR surface);
	void Destroy();

	VkPhysicalDevice m_physicalDeviceHandle;
	VkPhysicalDeviceProperties m_physicalDeviceProps;
	VkDevice m_deviceHandle;

	VulkanQueueFamilies m_queueFamilies;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	VkSampleCountFlags m_maxNumMsaaSamples;
	VkFormat m_depthFormat;
};
