#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Render/Vulkan/VulkanCommandPool.h"
#include "Engine/Core/Types.h"
#include <vector>

struct VulkanSwapchainDetails 
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class Window;

class VulkanSwapchain
{
public:
	VulkanSwapchain();

	void Init(Window* pWindow, const VkSurfaceKHR& surface);
	void Destroy();

	void AddInitialImageLayoutTransitionCommands(VkCommandBuffer commandBuffer);

	static VulkanSwapchainDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

	VkSwapchainKHR m_swapchainHandle;
	VkFormat m_format;
	VkExtent2D m_extent;
	U32 m_numImages;
	std::vector<VkImage> m_images;
	std::vector<VkFence> m_imageInFlightFences;
};
