#include "engine/render/vulkan/VulkanSwapchain.h"
#include "engine/render/vulkan/VulkanDevice.h"
#include "engine/render/vulkan/VulkanSurface.h"
#include "engine/render/vulkan/VulkanUtils.h"
#include "engine/core/Assert.h"

VulkanSwapchain::VulkanSwapchain(VulkanDevice* pDevice)
	:m_pDevice(pDevice)
	,m_vkSwapchain(VK_NULL_HANDLE)
	,m_numImages(0)
	,m_vkSwapchainFormat(VK_FORMAT_UNDEFINED)
	,m_vkSwapchainExtent({0, 0})
{
}

VulkanSwapchain::~VulkanSwapchain()
{
}

bool VulkanSwapchain::Setup()
{
	SwapchainSupportDetails swapChainDetails = m_pDevice->QuerySwapchainSupport();

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapchainSurfaceFormat(swapChainDetails.m_formats);
	VkPresentModeKHR presentMode = ChooseSwapchainPresentMode(swapChainDetails.m_presentModes);
	VkExtent2D imageExtent = ChooseSwapchainExtent(swapChainDetails.m_capabilities, m_pDevice->GetSurface()->GetWindowWidth(), m_pDevice->GetSurface()->GetWindowHeight());

	U32 imageCount = swapChainDetails.m_capabilities.minImageCount + 1; // one extra image to prevent waiting on driver
	if (swapChainDetails.m_capabilities.maxImageCount > 0)
	{
		imageCount = Min(imageCount, swapChainDetails.m_capabilities.maxImageCount);
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.surface = m_pDevice->GetSurface()->GetHandle();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.presentMode = presentMode;
	createInfo.imageExtent = imageExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = m_pDevice->GetQueueFamilyIndices();
	U32 queueFamilyIndices[] = { (U32)indices.m_graphicsFamily, (U32)indices.m_presentationFamily };

	if (indices.m_graphicsFamily != indices.m_presentationFamily)
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

	createInfo.preTransform = swapChainDetails.m_capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VULKAN_ASSERT(vkCreateSwapchainKHR(m_pDevice->GetHandle(), &createInfo, nullptr, &m_vkSwapchain));

	vkGetSwapchainImagesKHR(m_pDevice->GetHandle(), m_vkSwapchain, &m_numImages, nullptr);

	m_vkSwapchainImages.resize(m_numImages);
	vkGetSwapchainImagesKHR(m_pDevice->GetHandle(), m_vkSwapchain, &m_numImages, m_vkSwapchainImages.data());

	m_vkSwapchainFormat = surfaceFormat.format;
	m_vkSwapchainExtent = imageExtent;

	// create swapchain image views
	m_vkSwapchainImageViews.resize(m_vkSwapchainImages.size());
	for (size_t i = 0; i < m_vkSwapchainImages.size(); i++)
	{
		m_vkSwapchainImageViews[i] = m_pDevice->CreateImageView(m_vkSwapchainImages[i], m_vkSwapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
	
	return true;
}

void VulkanSwapchain::Teardown()
{
	for (VkImageView imageView : m_vkSwapchainImageViews)
	{
		vkDestroyImageView(m_pDevice->GetHandle(), imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_pDevice->GetHandle(), m_vkSwapchain, nullptr);
}

VkSwapchainKHR VulkanSwapchain::GetHandle()
{
	return m_vkSwapchain;
}

U32 VulkanSwapchain::GetNumImages()
{
	return m_numImages;
}

VkImage VulkanSwapchain::GetImage(U32 index)
{
	ASSERT(index < m_numImages);
	return m_vkSwapchainImages[index];
}

VkImageView VulkanSwapchain::GetImageView(U32 index)
{
	ASSERT(index < m_numImages);
	return m_vkSwapchainImageViews[index];
}

VkFormat VulkanSwapchain::GetFormat() const
{
	return m_vkSwapchainFormat;
}

VkExtent2D VulkanSwapchain::GetExtent() const
{
	return m_vkSwapchainExtent;
}

U32 VulkanSwapchain::GetWidth() const
{
	return m_vkSwapchainExtent.width;
}

U32 VulkanSwapchain::GetHeight() const
{
	return m_vkSwapchainExtent.height;
}

F32 VulkanSwapchain::GetAspectRatio() const
{
	VkExtent2D extent = GetExtent();
	return static_cast<F32>(extent.width) / static_cast<F32>(extent.height);
}
