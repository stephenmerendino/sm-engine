#include "Engine/Render/Vulkan/VulkanSwapchain.h"
#include "Engine/Core/Assert.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Render/Vulkan/VulkanCommands.h"
#include "Engine/Render/Window.h"

static VkSurfaceFormatKHR ChooseSurfaceFormat(std::vector<VkSurfaceFormatKHR> formats)
{
	for (const VkSurfaceFormatKHR& format : formats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return formats[0];
}

static VkPresentModeKHR ChoosePresentMode(std::vector<VkPresentModeKHR> presentModes)
{
	//VK_PRESENT_MODE_IMMEDIATE_KHR = 0,
	//VK_PRESENT_MODE_MAILBOX_KHR = 1,
	//VK_PRESENT_MODE_FIFO_KHR = 2,
	//VK_PRESENT_MODE_FIFO_RELAXED_KHR = 3,
	//VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR = 1000111000,
	//VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR = 1000111001,
	for (const VkPresentModeKHR& mode : presentModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, U32 windowWidth, U32 windowHeight)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}

	VkExtent2D actualExtent = { windowWidth, windowHeight };

	actualExtent.width = Clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = Clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}

VulkanSwapchainDetails VulkanSwapchain::QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	VulkanSwapchainDetails details;

	// surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

	// surface formats
	U32 numSurfaceFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &numSurfaceFormats, nullptr);

	if (numSurfaceFormats != 0)
	{
		details.formats.resize(numSurfaceFormats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &numSurfaceFormats, details.formats.data());
	}

	// present modes
	U32 numPresentModes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &numPresentModes, nullptr);

	if (numPresentModes != 0)
	{
		details.presentModes.resize(numPresentModes);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &numPresentModes, details.presentModes.data());
	}

	return details;
}

VulkanSwapchain::VulkanSwapchain()
	:m_swapchainHandle(VK_NULL_HANDLE)
	,m_format(VK_FORMAT_UNDEFINED)
	,m_extent({0,0})
	,m_numImages(0)
{
}

void VulkanSwapchain::Init(Window* pWindow, const VkSurfaceKHR& surface)
{
	SM_ASSERT(pWindow != nullptr);

	VulkanSwapchainDetails swapchainDetails = QuerySwapchainSupport(VulkanDevice::GetPhysDeviceHandle(), surface);

	VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(swapchainDetails.formats);
	VkPresentModeKHR presentMode = ChoosePresentMode(swapchainDetails.presentModes);
	VkExtent2D imageExtent = ChooseExtent(swapchainDetails.capabilities, pWindow->m_width, pWindow->m_height);

	U32 image_count = swapchainDetails.capabilities.minImageCount + 1; // one extra image to prevent waiting on driver
	if (swapchainDetails.capabilities.maxImageCount > 0)
	{
		image_count = Min(image_count, swapchainDetails.capabilities.maxImageCount);
	}

	// TODO: Allow fullscreen
	//VkSurfaceFullScreenExclusiveInfoEXT fullScreenInfo = {};
	//fullScreenInfo.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
	//fullScreenInfo.pNext = nullptr;
	//fullScreenInfo.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.pNext = nullptr; // use full_screen_info here
	createInfo.surface = surface;
	createInfo.minImageCount = image_count;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.presentMode = presentMode;
	createInfo.imageExtent = imageExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	const VulkanQueueFamilies& queueFamilies = VulkanDevice::Get()->m_queueFamilies;
	U32 queueFamilyIndices[] = { (U32)queueFamilies.m_graphicsFamilyIndex, (U32)queueFamilies.m_presentFamilyIndex};

	if (queueFamilies.m_graphicsFamilyIndex != queueFamilies.m_presentFamilyIndex)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapchainDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	SM_VULKAN_ASSERT(vkCreateSwapchainKHR(VulkanDevice::GetHandle(), &createInfo, nullptr, &m_swapchainHandle));

	vkGetSwapchainImagesKHR(VulkanDevice::GetHandle(), m_swapchainHandle, &m_numImages, nullptr);

	m_images.resize(m_numImages);
	vkGetSwapchainImagesKHR(VulkanDevice::GetHandle(), m_swapchainHandle, &m_numImages, m_images.data());

	m_format = surfaceFormat.format;
	m_extent = imageExtent;
	//m_imageInFlightFences.resize(m_numImages);
}

void VulkanSwapchain::Destroy()
{
	vkDestroySwapchainKHR(VulkanDevice::GetHandle(), m_swapchainHandle, nullptr);
}

void VulkanSwapchain::AddInitialImageLayoutTransitionCommands(VkCommandBuffer commandBuffer)
{
	for (I32 i = 0; i < (I32)m_numImages; i++)
	{
		VulkanCommands::TransitionImageLayout(commandBuffer, m_images[i], 1, 
															 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
															 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
															 VK_ACCESS_NONE, VK_ACCESS_NONE);
	}
}

U32 VulkanSwapchain::AcquireNextImage()
{
	U32 imageIndex;
	vkAcquireNextImageKHR(VulkanDevice::GetHandle(), m_swapchainHandle, UINT64_MAX, m_imageIsReadySemaphore[imageIndex].m_semaphoreHandle, VK_NULL_HANDLE, &imageIndex);
	return imageIndex;
}