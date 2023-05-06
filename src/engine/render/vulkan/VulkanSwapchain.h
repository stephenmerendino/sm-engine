#pragma once

#include "engine/render/vulkan/VulkanInclude.h"
#include "engine/core/Types.h"

#include <vector>

class VulkanDevice;

class VulkanSwapchain
{
public:
	VulkanSwapchain(VulkanDevice* pDevice);
	~VulkanSwapchain();

	bool Setup();
	void Teardown();
	
	VkSwapchainKHR GetHandle();
	U32 GetNumImages();
	VkImage GetImage(U32 index);
	VkImageView GetImageView(U32 index);
	VkFormat GetFormat() const;
	VkExtent2D GetExtent() const;
	U32 GetWidth() const;
	U32 GetHeight() const;
	F32 GetAspectRatio() const;

private:
	VulkanDevice* m_pDevice;
	VkSwapchainKHR m_vkSwapchain;
	U32 m_numImages;
	std::vector<VkImage> m_vkSwapchainImages;
	std::vector<VkImageView> m_vkSwapchainImageViews;
	VkFormat m_vkSwapchainFormat;
	VkExtent2D m_vkSwapchainExtent;
};
