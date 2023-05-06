#pragma once

#include "engine/core/Types.h"
#include "engine/render/vulkan/VulkanInclude.h"

#include <vector>

class VulkanSurface;

struct QueueFamilyIndices
{
	static const I32 NO_QUEUE_INDEX = -1;

	I32 m_graphicsFamily = NO_QUEUE_INDEX;
	I32 m_presentationFamily = NO_QUEUE_INDEX;

	bool IsComplete();
};

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR m_capabilities;
	std::vector<VkSurfaceFormatKHR> m_formats;
	std::vector<VkPresentModeKHR> m_presentModes;
};

class VulkanDevice
{
public:
	VulkanDevice(VulkanSurface* pSurface);
	~VulkanDevice();

	void Setup();
	void Teardown();

	VulkanSurface* GetSurface();
	VkDevice GetHandle();
	VkPhysicalDevice GetPhysicalDeviceHandle();
	VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const;
	VkQueue GetGraphicsQueue();
	VkQueue GetPresentationQueue();
	QueueFamilyIndices GetQueueFamilyIndices();
	
	U32 FindMemoryType(U32 typeFilter, VkMemoryPropertyFlags properties);
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat FindDepthFormat();

	SwapchainSupportDetails QuerySwapchainSupport();

	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, U32 numMipLevels);
	VkCommandPool CreateCommandPool(VkQueueFlags queueFamilies, VkCommandPoolCreateFlags createFlags);
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer& outBuffer, VkDeviceMemory& outBufferMemory);
	VkShaderModule CreateShaderModule(const char* shaderFilepath);
	void CreateImage(int texWidth, int texHeight, int numMipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memProps, VkImage& vkTextureImage, VkDeviceMemory& vkTextureImageMemory);
	void CopyIntoMemory(void* data, size_t size, VkDeviceMemory gpuMemory, VkDeviceSize gpuMemoryOffset = 0);

	void Flush();
	void FlushGraphics();

	VkSampleCountFlagBits GetMaxMsaaSampleCount();

private:
	VulkanSurface* m_pSurface;
	VkPhysicalDevice m_vkPhysicalDevice;
	VkPhysicalDeviceProperties m_vkPhysicalDeviceProps;
	VkDevice m_vkDevice;
	VkQueue m_vkGraphicsQueue;
	VkQueue m_vkPresentationQueue;
	VkSampleCountFlagBits m_vkMaxNumMsaaSamples;
	QueueFamilyIndices m_queueFamilyIndices;

	void PickPhysicalDevice();
	void CreateLogicalDevice();
	VkSampleCountFlagBits QueryMaxMsaaSampleCount();
};
