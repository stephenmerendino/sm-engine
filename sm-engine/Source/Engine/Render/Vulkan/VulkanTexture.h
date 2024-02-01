#pragma once

#include "Engine/Render/Vulkan/VulkanCommandPool.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Core/Types.h"

class VulkanTexture
{
public:
	VulkanTexture();

	void InitFromFile(const VulkanDevice& device, const VulkanCommandPool& commandPool, const char* filepath);
	void InitColorTarget(const VulkanDevice& device, VkFormat format, U32 width, U32 height, VkImageUsageFlags usage, VkSampleCountFlagBits numSamples);
	void InitDepthTarget(const VulkanDevice& device, VkFormat format, U32 width, U32 height, VkImageUsageFlags usage, VkSampleCountFlagBits numSamples);

	void Destroy();

	const VulkanDevice* m_device;
	VkImage m_image;
	VkImageView m_imageView;
	VkDeviceMemory m_deviceMemory;
	U32 m_numMips;
};
