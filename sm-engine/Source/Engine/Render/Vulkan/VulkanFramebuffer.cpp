#include "Engine/Render/Vulkan/VulkanFramebuffer.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"

VulkanFramebuffer::VulkanFramebuffer()
	:m_framebufferHandle(VK_NULL_HANDLE)
{
}

void VulkanFramebuffer::Init(const VulkanRenderPass& renderPass, const std::vector<VkImageView>& attachments, U32 width, U32 height, U32 layers)
{
	VkFramebufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.renderPass = renderPass.m_renderPassHandle;
	createInfo.pAttachments = attachments.data();
	createInfo.attachmentCount = (U32)attachments.size();
	createInfo.width = width;
	createInfo.height = height;
	createInfo.layers = layers;

	SM_VULKAN_ASSERT_OLD(vkCreateFramebuffer(VulkanDevice::GetHandle(), &createInfo, nullptr, &m_framebufferHandle));
}

void VulkanFramebuffer::Destroy()
{
	vkDestroyFramebuffer(VulkanDevice::GetHandle(), m_framebufferHandle, nullptr);
}