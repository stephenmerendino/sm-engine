#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanQueueFamilies.h"
#include <vector>

class VulkanDevice
{
public:
	static void Init(VkSurfaceKHR surface);
	static void Destroy();
	static VulkanDevice* Get();
	static VkDevice GetHandle();
	static VkPhysicalDevice GetPhysDeviceHandle();

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

	VkSampleCountFlagBits m_maxNumMsaaSamples;

private:
	VulkanDevice();

	static VulkanDevice* s_device;
};
