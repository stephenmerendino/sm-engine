#include "Engine/Render/Vulkan/VulkanUtils.h"
#include "Engine/Core/Common.h"
#include "Engine/Math/MathUtils.h"

VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat(std::vector<VkSurfaceFormatKHR> formats)
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

VkPresentModeKHR ChooseSwapchainPresentMode(std::vector<VkPresentModeKHR> presentModes)
{
	for (const VkPresentModeKHR& presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities, U32 windowWidth, U32 windowHeight)
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

bool HasStencilComponent(VkFormat format)
{
	return (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || 
		   (format == VK_FORMAT_D24_UNORM_S8_UINT);
}

VkDebugUtilsMessengerCreateInfoEXT SetupDebugMessengerCreateInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback)
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = callback;
	createInfo.pUserData = nullptr;

	return createInfo;
}

VkAttachmentDescription SetupAttachmentDescription(VkFormat format, VkSampleCountFlagBits sampleCount, 
												   VkImageLayout initialLayout, VkImageLayout finalLayout,
												   VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, 
												   VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp, 
												   VkAttachmentDescriptionFlags flags)
{
	VkAttachmentDescription attachmentDesc{};
	attachmentDesc.format = format;
	attachmentDesc.samples = sampleCount;
	attachmentDesc.loadOp = loadOp;
	attachmentDesc.storeOp = storeOp;
	attachmentDesc.stencilLoadOp = stencilLoadOp;
	attachmentDesc.stencilStoreOp = stencilStoreOp;
	attachmentDesc.initialLayout = initialLayout;
	attachmentDesc.finalLayout = finalLayout;
	attachmentDesc.flags = flags;
	return attachmentDesc;
}