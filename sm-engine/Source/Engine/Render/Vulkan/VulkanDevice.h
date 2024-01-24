#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanQueueFamilies.h"

class VulkanDevice
{
public:
	VulkanDevice();

	void Init(VkInstance instance, VkSurfaceKHR surface);
	void Destroy();

	VkPhysicalDevice m_physicalDevice;
	VkPhysicalDeviceProperties m_physicalDeviceProps;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;
	VkSampleCountFlags m_maxNumMsaaSamples;
	VulkanQueueFamilies m_queueFamilies;
	VkFormat m_depthFormat;
};
