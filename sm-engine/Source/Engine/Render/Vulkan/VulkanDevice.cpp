#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Render/Vulkan/VulkanFormats.h"
#include "Engine/Render/Vulkan/VulkanInstance.h"
#include "Engine/Render/Vulkan/VulkanSwapchain.h"
#include "Engine/Config_old.h"
#include "Engine/Core/Assert_old.h"
#include "Engine/Core/Debug_Old.h"
#include <vector>
#include <set>

VulkanDevice* VulkanDevice::s_device = nullptr;

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

	VulkanSwapchainDetails swapchainDetails = VulkanSwapchain::QuerySwapchainSupport(device, surface);
	if (swapchainDetails.formats.empty() || swapchainDetails.presentModes.empty())
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

VulkanDevice::VulkanDevice()
	:m_physicalDeviceHandle(VK_NULL_HANDLE)
	,m_physicalDeviceProps({})
	,m_physicalDeviceMemoryProps({})
	,m_deviceHandle(VK_NULL_HANDLE)
	,m_graphicsQueue(VK_NULL_HANDLE)
	,m_presentQueue(VK_NULL_HANDLE)
	,m_maxNumMsaaSamples(VK_SAMPLE_COUNT_1_BIT)
{
}

void VulkanDevice::Init(VkSurfaceKHR surface)
{
	// physical device
	VkPhysicalDevice selecetedPhysicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties selectedPhysicalDeviceProps = {};
	VkPhysicalDeviceMemoryProperties selectedPhysicalDeviceMemoryProps = {};
	VulkanQueueFamilies queueFamilies;
	VkSampleCountFlagBits maxNumMsaaSamples = VK_SAMPLE_COUNT_1_BIT;

	{
		U32 deviceCount = 0;
		vkEnumeratePhysicalDevices(VulkanInstance::GetHandle(), &deviceCount, nullptr);
		SM_ASSERT_OLD(deviceCount != 0);

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(VulkanInstance::GetHandle(), &deviceCount, devices.data());

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
				vkGetPhysicalDeviceMemoryProperties(selecetedPhysicalDevice, &selectedPhysicalDeviceMemoryProps);
				queueFamilies.Init(selecetedPhysicalDevice, surface);
				maxNumMsaaSamples = GetMaxMsaaSampleCount(selectedPhysicalDeviceProps);
				break;
			}
		}

		SM_ASSERT_OLD(VK_NULL_HANDLE != selecetedPhysicalDevice);
	}

	// logical device
	VkDevice logicalDevice = VK_NULL_HANDLE;
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<I32> uniqueQueueFamilies = { queueFamilies.m_graphicsAndComputeFamilyIndex, queueFamilies.m_presentationFamilyIndex };

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

		SM_VULKAN_ASSERT_OLD(vkCreateDevice(selecetedPhysicalDevice, &createInfo, nullptr, &logicalDevice));

		LoadVulkanDeviceFuncs(logicalDevice);
	}

	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
	{
		vkGetDeviceQueue(logicalDevice, queueFamilies.m_graphicsAndComputeFamilyIndex, 0, &graphicsQueue);
		vkGetDeviceQueue(logicalDevice, queueFamilies.m_presentationFamilyIndex, 0, &presentQueue);
	}

	s_device = new VulkanDevice();
	s_device->m_physicalDeviceHandle = selecetedPhysicalDevice;
	s_device->m_physicalDeviceProps = selectedPhysicalDeviceProps;
	s_device->m_physicalDeviceMemoryProps = selectedPhysicalDeviceMemoryProps;
	s_device->m_deviceHandle = logicalDevice;
	s_device->m_graphicsQueue = graphicsQueue;
	s_device->m_presentQueue = presentQueue;
	s_device->m_maxNumMsaaSamples = maxNumMsaaSamples;
	s_device->m_queueFamilies = queueFamilies;
}

void VulkanDevice::Destroy()
{
	vkDestroyDevice(s_device->m_deviceHandle, nullptr);
	delete s_device;
}

VulkanDevice* VulkanDevice::Get()
{
	return s_device;
}

VkDevice VulkanDevice::GetHandle()
{
	return s_device->m_deviceHandle;
}

VkPhysicalDevice VulkanDevice::GetPhysDeviceHandle()
{
	return s_device->m_physicalDeviceHandle;
}

void VulkanDevice::FlushPipe()
{
	vkDeviceWaitIdle(m_deviceHandle);
}

VkFormat VulkanDevice::FindSupportedDepthFormat() const
{
	VkFormat depthFormat = FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
												VK_IMAGE_TILING_OPTIMAL,
												VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	SM_ASSERT_OLD(depthFormat != VK_FORMAT_UNDEFINED);
	return depthFormat;
}

VkFormat VulkanDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_physicalDeviceHandle, format, &props);

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

U32 VulkanDevice::FindSupportedMemoryType(U32 typeFilter, VkMemoryPropertyFlags properties) const
{
	U32 foundMemType = UINT_MAX;
	for (U32 i = 0; i < m_physicalDeviceMemoryProps.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (m_physicalDeviceMemoryProps.memoryTypes[i].propertyFlags & properties) == properties)
		{
			foundMemType = i;
			break;
		}
	}

	SM_ASSERT_OLD(foundMemType != UINT_MAX);
	return foundMemType;
}

VkFormatProperties VulkanDevice::QueryFormatProperties(VkFormat format) const
{
	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(m_physicalDeviceHandle, format, &formatProps);
	return formatProps;
}
