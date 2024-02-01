#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanQueueFamilies.h"
#include <vector>

class VulkanDevice
{
public:
	VulkanDevice();

	void Init(VkInstance instance, VkSurfaceKHR surface);
	void Destroy();

	VkFormat FindSupportedDepthFormat() const;
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
	U32 FindSupportedMemoryType(U32 typeFilter, VkMemoryPropertyFlags properties) const;
	VkFormatProperties QueryFormatProperties(VkFormat format) const;

	VkPhysicalDevice m_physicalDeviceHandle;
	VkPhysicalDeviceProperties m_physicalDeviceProps;
	VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProps;
	VkDevice m_deviceHandle;

	VulkanQueueFamilies m_queueFamilies;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	VkSampleCountFlags m_maxNumMsaaSamples;
};
