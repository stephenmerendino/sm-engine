#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanRenderPass.h"

class VulkanDevice;

class VulkanFramebuffer
{
public:
	VulkanFramebuffer();

	void Init(const VulkanRenderPass& renderPass, const std::vector<VkImageView>& attachments, U32 width, U32 height, U32 layers);
	void Destroy();

	VkFramebuffer m_framebufferHandle;
};
