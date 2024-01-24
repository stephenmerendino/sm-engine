#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Config.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Debug.h"
#include <vector>
#include <set>

struct VulkanSwapchainDetails 
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

static VkSampleCountFlagBits GetMaxMsaaSampleCount(VkPhysicalDeviceProperties props)
{
	VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
	if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
	if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
	if (counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
	if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
	if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
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

static VulkanSwapchainDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	VulkanSwapchainDetails details;

	// surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	// surface formats
	U32 numSurfaceFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numSurfaceFormats, nullptr);

	if (numSurfaceFormats != 0)
	{
		details.formats.resize(numSurfaceFormats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numSurfaceFormats, details.formats.data());
	}

	// present modes
	U32 numPresentModes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numPresentModes, nullptr);

	if (numPresentModes != 0)
	{
		details.present_modes.resize(numPresentModes);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numPresentModes, details.present_modes.data());
	}

	return details;
}

static bool IsPhysicalDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);

	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);

	// TODO: Support integrated gpus in the future
	if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
	{
		return false;
	}

	if (features.samplerAnisotropy == VK_FALSE)
	{
		return false;
	}

	bool supportsExtensions = CheckPhysicalDeviceExtensionSupport(device);
	if (!supportsExtensions)
	{
		return false;
	}

	VulkanSwapchainDetails swapchainDetails = QuerySwapchainSupport(device, surface);
	if (swapchainDetails.formats.empty() || swapchainDetails.present_modes.empty())
	{
		return false;
	}

	VulkanQueueFamilies queueFamilies;
	queueFamilies.Init(device, surface);
	if (!queueFamilies.HasRequiredFamilies())
	{
		return false;
	}

	return true;
}

static bool FormatHasStencilComponent(VkFormat format)
{
	return (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || 
		   (format == VK_FORMAT_D24_UNORM_S8_UINT);
}

static VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

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

static VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice)
{
	VkFormat depth_format = FindSupportedFormat(physicalDevice,
												{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
												VK_IMAGE_TILING_OPTIMAL,
												VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	SM_ASSERT(depth_format != VK_FORMAT_UNDEFINED);
	return depth_format;
}

VulkanDevice::VulkanDevice()
	:m_physicalDevice(VK_NULL_HANDLE)
	,m_physicalDeviceProps({})
	,m_device(VK_NULL_HANDLE)
	,m_graphicsQueue(VK_NULL_HANDLE)
	,m_presentQueue(VK_NULL_HANDLE)
	,m_maxNumMsaaSamples(VK_SAMPLE_COUNT_1_BIT)
	,m_depthFormat(VK_FORMAT_UNDEFINED)
{
}

void VulkanDevice::Init(VkInstance instance, VkSurfaceKHR surface)
{
	// physical device
	VkPhysicalDevice selecetedPhysicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties selectedPhysicalDeviceProps = {};
	VulkanQueueFamilies queueFamilies;
	VkSampleCountFlagBits maxNumMsaaSamples = VK_SAMPLE_COUNT_1_BIT;

	{
		U32 deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		SM_ASSERT(deviceCount != 0);

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		if (IsDebug())
		{
			DebugPrintf("Physical Devices:\n");

			for (const VkPhysicalDevice& device : devices)
			{
				VkPhysicalDeviceProperties deviceProps;
				vkGetPhysicalDeviceProperties(device, &deviceProps);

				//VkPhysicalDeviceFeatures deviceFeatures;
				//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

				DebugPrintf("%s\n", deviceProps.deviceName);
			}
		}

		for (const VkPhysicalDevice& device : devices)
		{
			if (IsPhysicalDeviceSuitable(device, surface))
			{
				selecetedPhysicalDevice = device;
				vkGetPhysicalDeviceProperties(selecetedPhysicalDevice, &selectedPhysicalDeviceProps);
				queueFamilies.Init(selecetedPhysicalDevice, surface);
				maxNumMsaaSamples = GetMaxMsaaSampleCount(selectedPhysicalDeviceProps);
				break;
			}
		}

		SM_ASSERT(VK_NULL_HANDLE != selecetedPhysicalDevice);
	}

	// logical device
	VkDevice logicalDevice = VK_NULL_HANDLE;
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<I32> uniqueQueueFamilies = { queueFamilies.m_graphicsFamily, queueFamilies.m_presentFamily };

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

		VkPhysicalDeviceVulkan13Features vk13features{}; // vulkan 1.3 features
		vk13features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		vk13features.synchronization2 = VK_TRUE;

		createInfo.pNext = &vk13features;

		SM_VULKAN_ASSERT(vkCreateDevice(selecetedPhysicalDevice, &createInfo, nullptr, &logicalDevice));

		LoadVulkanDeviceFuncs(logicalDevice);
	}

	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
	{
		vkGetDeviceQueue(logicalDevice, queueFamilies.m_graphicsFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(logicalDevice, queueFamilies.m_presentFamily, 0, &presentQueue);
	}

	m_physicalDevice = selecetedPhysicalDevice;
	m_device = logicalDevice;
	m_graphicsQueue = graphicsQueue;
	m_presentQueue = presentQueue;
	m_maxNumMsaaSamples = maxNumMsaaSamples;
	m_queueFamilies = queueFamilies;
	m_physicalDeviceProps = selectedPhysicalDeviceProps;
	m_depthFormat = FindDepthFormat(m_physicalDevice);
}

void VulkanDevice::Destroy()
{
	vkDestroyDevice(m_device, nullptr);
}