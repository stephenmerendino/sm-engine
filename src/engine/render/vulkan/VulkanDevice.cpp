#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Render/Vulkan/VulkanInstance.h"
#include "Engine/Render/Vulkan/VulkanSurface.h"
#include "Engine/Core/Types.h"
#include "Engine/Core/Common.h"
#include "Engine/Core/FileUtils.h"
#include "Engine/Core/Config.h"

#include <vector>
#include <set>
#include <string>

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR m_capabilities;
	std::vector<VkSurfaceFormatKHR> m_formats;
	std::vector<VkPresentModeKHR> m_presentModes;
};

static void DebugPrintPhysicalDevice(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProps;
	vkGetPhysicalDeviceProperties(device, &deviceProps);

	//VkPhysicalDeviceFeatures deviceFeatures;
	//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	DebugPrintf("%s\n", deviceProps.deviceName);
}

static bool CheckPhysicalDeviceExtensionSupport(VkPhysicalDevice device)
{
	U32 numExtensions = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, nullptr);

	std::vector<VkExtensionProperties> extensions(numExtensions);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, extensions.data());

	std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
	for (const VkExtensionProperties& extension : extensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	SwapChainSupportDetails details;

	// surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.m_capabilities);

	// surface formats
	U32 numSurfaceFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numSurfaceFormats, nullptr);

	if (numSurfaceFormats != 0)
	{
		details.m_formats.resize(numSurfaceFormats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numSurfaceFormats, details.m_formats.data());
	}

	// present modes
	U32 numPresentModes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numPresentModes, nullptr);

	if (numPresentModes != 0)
	{
		details.m_presentModes.resize(numPresentModes);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numPresentModes, details.m_presentModes.data());
	}

	return details;
}

static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices;

	U32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProps.data());

	for (int i = 0; i < (int)queueFamilyProps.size(); i++)
	{
		const VkQueueFamilyProperties& props = queueFamilyProps[i];
		if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.m_graphicsFamily = i;
		}

		// query for presentation support on this queue
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);

		if (presentationSupport)
		{
			indices.m_presentationFamily = i;
		}

		if (indices.IsComplete())
		{
			break;
		}
	}

	return indices;
}

static bool IsPhysicalDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	VkPhysicalDeviceProperties deviceProps;
	vkGetPhysicalDeviceProperties(device, &deviceProps);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
	{
		return false;
	}

	if (deviceFeatures.samplerAnisotropy == VK_FALSE)
	{
		return false;
	}

	bool supportsExtensions = CheckPhysicalDeviceExtensionSupport(device);
	if (!supportsExtensions)
	{
		return false;
	}

	SwapChainSupportDetails swapChainDetails = QuerySwapChainSupport(device, surface);
	if (swapChainDetails.m_formats.empty() || swapChainDetails.m_presentModes.empty())
	{
		return false;
	}

	QueueFamilyIndices queueIndices = FindQueueFamilies(device, surface);
	if (!queueIndices.IsComplete())
	{
		return false;
	}

	return true;
}

bool QueueFamilyIndices::IsComplete()
{
	return m_graphicsFamily != NO_QUEUE_INDEX &&
		   m_presentationFamily != NO_QUEUE_INDEX;
}

VulkanDevice::VulkanDevice(VulkanSurface* surface)
	:m_pSurface(surface)
	,m_vkPhysicalDevice(VK_NULL_HANDLE)
	,m_vkPhysicalDeviceProps({})
	,m_vkDevice(VK_NULL_HANDLE)
	,m_vkGraphicsQueue(VK_NULL_HANDLE)
	,m_vkPresentationQueue(VK_NULL_HANDLE)
	,m_vkMaxNumMsaaSamples(VK_SAMPLE_COUNT_1_BIT)
{
}

VulkanDevice::~VulkanDevice()
{
}

void VulkanDevice::Setup()
{
	PickPhysicalDevice();
	CreateLogicalDevice();
}

void VulkanDevice::Teardown()
{
	vkDestroyDevice(m_vkDevice, nullptr);
}

VulkanSurface* VulkanDevice::GetSurface()
{
	return m_pSurface;
}

VkDevice VulkanDevice::GetHandle()
{
	return m_vkDevice;
}

VkPhysicalDevice VulkanDevice::GetPhysicalDeviceHandle()
{
	return m_vkPhysicalDevice;
}

VkPhysicalDeviceProperties VulkanDevice::GetPhysicalDeviceProperties() const
{
	return m_vkPhysicalDeviceProps;
}

void VulkanDevice::PickPhysicalDevice()
{
	U32 deviceCount = 0;
	vkEnumeratePhysicalDevices(VulkanInstance::GetHandle(), &deviceCount, nullptr);
	ASSERT(deviceCount != 0);

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(VulkanInstance::GetHandle(), &deviceCount, devices.data());

	if (IsDebug())
	{
		DebugPrintf("Physical Devices:\n");
		for (const VkPhysicalDevice& device : devices)
		{
			DebugPrintPhysicalDevice(device);
		}
	}

	for (const VkPhysicalDevice& device : devices)
	{
		if (IsPhysicalDeviceSuitable(device, m_pSurface->GetHandle()))
		{
			m_vkPhysicalDevice = device;
			vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &m_vkPhysicalDeviceProps);
			m_queueFamilyIndices = FindQueueFamilies(m_vkPhysicalDevice, m_pSurface->GetHandle());
			m_vkMaxNumMsaaSamples = QueryMaxMsaaSampleCount();
			break;
		}
	}

	ASSERT(VK_NULL_HANDLE != m_vkPhysicalDevice);
}

void VulkanDevice::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<I32> uniqueQueueFamilies = { m_queueFamilyIndices.m_graphicsFamily, m_queueFamilyIndices.m_presentationFamily };

	F32 queuePriority = 1.0f;
	for (I32 queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};

		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = (U32)queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = (U32)DEVICE_EXTENSIONS.size();
	createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

	// validation layers are not used on devices anymore, but this is for older vulkan systems that still used it
	if (ENABLE_VALIDATION_LAYERS)
	{
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		createInfo.enabledLayerCount = (U32)VALIDATION_LAYERS.size();
	}

	VULKAN_ASSERT(vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice));

	LoadVulkanDeviceFuncs(m_vkDevice);

	vkGetDeviceQueue(m_vkDevice, m_queueFamilyIndices.m_graphicsFamily, 0, &m_vkGraphicsQueue);
	vkGetDeviceQueue(m_vkDevice, m_queueFamilyIndices.m_presentationFamily, 0, &m_vkPresentationQueue);
}

VkSampleCountFlagBits VulkanDevice::QueryMaxMsaaSampleCount()
{
	VkSampleCountFlags counts = m_vkPhysicalDeviceProps.limits.framebufferColorSampleCounts & m_vkPhysicalDeviceProps.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
	if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
	if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
	if (counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
	if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
	if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
}

void VulkanDevice::Flush()
{
	ASSERT(m_vkDevice != VK_NULL_HANDLE);
	vkDeviceWaitIdle(m_vkDevice);
}

void VulkanDevice::FlushGraphics()
{
	ASSERT(m_vkGraphicsQueue != VK_NULL_HANDLE);
	vkQueueWaitIdle(m_vkGraphicsQueue);
}

VkSampleCountFlagBits VulkanDevice::GetMaxMsaaSampleCount()
{
	return m_vkMaxNumMsaaSamples;
}

VkQueue VulkanDevice::GetGraphicsQueue()
{
	return m_vkGraphicsQueue;
}

VkQueue VulkanDevice::GetPresentationQueue()
{
	return m_vkPresentationQueue;
}

QueueFamilyIndices VulkanDevice::GetQueueFamilyIndices()
{
	return m_queueFamilyIndices;
}

U32 VulkanDevice::FindMemoryType(U32 typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &memProperties);

	U32 foundMemType = UINT_MAX;
	for (U32 i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			foundMemType = i;
			break;
		}
	}

	ASSERT(foundMemType != UINT_MAX);
	return foundMemType;
}

VkFormat VulkanDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(GetPhysicalDeviceHandle(), format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	return VK_FORMAT_UNDEFINED;
}

VkFormat VulkanDevice::FindDepthFormat()
{
	VkFormat depthFormat = FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
												VK_IMAGE_TILING_OPTIMAL, 
												VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	ASSERT(depthFormat != VK_FORMAT_UNDEFINED);
	return depthFormat;
}

SwapchainSupportDetails VulkanDevice::QuerySwapchainSupport()
{
	SwapchainSupportDetails details;

	// surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevice, m_pSurface->GetHandle(), &details.m_capabilities);

	// surface formats
	U32 numSurfaceFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_pSurface->GetHandle(), &numSurfaceFormats, nullptr);

	if (numSurfaceFormats != 0)
	{
		details.m_formats.resize(numSurfaceFormats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_pSurface->GetHandle(), &numSurfaceFormats, details.m_formats.data());
	}

	// present modes
	U32 numPresentModes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, m_pSurface->GetHandle(), &numPresentModes, nullptr);

	if (numPresentModes != 0)
	{
		details.m_presentModes.resize(numPresentModes);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, m_pSurface->GetHandle(), &numPresentModes, details.m_presentModes.data());
	}

	return details;
}

VkImageView VulkanDevice::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, U32 numMipLevels)
{
	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = numMipLevels;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	VULKAN_ASSERT(vkCreateImageView(m_vkDevice, &createInfo, nullptr, &imageView));
	return imageView;
}

VkCommandPool VulkanDevice::CreateCommandPool(VkQueueFlags queueFamilies, VkCommandPoolCreateFlags createFlags)
{
	QueueFamilyIndices queueFamilyIndices = GetQueueFamilyIndices();

	VkCommandPoolCreateInfo commandPoolInfo{};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.flags = createFlags;
	
	if (queueFamilies & VK_QUEUE_GRAPHICS_BIT)
	{
		commandPoolInfo.queueFamilyIndex = queueFamilyIndices.m_graphicsFamily;
	}
	
	VkCommandPool commandPool = VK_NULL_HANDLE;
	VULKAN_ASSERT(vkCreateCommandPool(m_vkDevice, &commandPoolInfo, nullptr, &commandPool));
	return commandPool;
}

void VulkanDevice::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer& outBuffer, VkDeviceMemory& outBufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usageFlags;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VULKAN_ASSERT(vkCreateBuffer(m_vkDevice, &bufferCreateInfo, nullptr, &outBuffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_vkDevice, outBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags);

	VULKAN_ASSERT(vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &outBufferMemory));

	vkBindBufferMemory(m_vkDevice, outBuffer, outBufferMemory, 0);
}

VkShaderModule VulkanDevice::CreateShaderModule(const char* shaderFilepath)
{
	std::vector<Byte> rawShaderCode = ReadBinaryFile(shaderFilepath);

	VkShaderModuleCreateInfo createInfo;
	MemZero(createInfo);
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = rawShaderCode.size();
	createInfo.pCode = (const U32*)rawShaderCode.data();

	VkShaderModule shaderModule = VK_NULL_HANDLE;
	VULKAN_ASSERT(vkCreateShaderModule(m_vkDevice, &createInfo, nullptr, &shaderModule));

	return shaderModule;
}

void VulkanDevice::CreateImage(int texWidth, int texHeight, int numMipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memProps, VkImage& vkTextureImage, VkDeviceMemory& vkTextureImageMemory)
{
	// Create vulkan image object
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = static_cast<U32>(texWidth);
	imageCreateInfo.extent.height = static_cast<U32>(texHeight);
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = numMipLevels;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = format;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = numSamples;
	imageCreateInfo.flags = 0;

	VULKAN_ASSERT(vkCreateImage(m_vkDevice, &imageCreateInfo, nullptr, &vkTextureImage));

	// Setup backing memory of image
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_vkDevice, vkTextureImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, memProps);

	VULKAN_ASSERT(vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &vkTextureImageMemory));

	vkBindImageMemory(m_vkDevice, vkTextureImage, vkTextureImageMemory, 0);
}

void VulkanDevice::CopyIntoMemory(void* data, size_t size, VkDeviceMemory gpuMemory, VkDeviceSize gpuMemoryOffset)
{
	void* mappedGpuMemory;
	vkMapMemory(m_vkDevice, gpuMemory, gpuMemoryOffset, size, 0, &mappedGpuMemory);
	memcpy(mappedGpuMemory, data, size);
	vkUnmapMemory(m_vkDevice, gpuMemory);
}