#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"
#include <vector>

VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat(std::vector<VkSurfaceFormatKHR> formats);
VkPresentModeKHR ChooseSwapchainPresentMode(std::vector<VkPresentModeKHR> presentModes);
VkExtent2D ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities, U32 windowWidth, U32 windowHeight);
bool HasStencilComponent(VkFormat format);

// Initalizers
VkDebugUtilsMessengerCreateInfoEXT SetupDebugMessengerCreateInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback);
VkAttachmentDescription SetupAttachmentDescription(VkFormat format, VkSampleCountFlagBits sampleCount, 
												   VkImageLayout initialLayout, VkImageLayout finalLayout,
												   VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, 
												   VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, 
												   VkAttachmentDescriptionFlags flags = 0);