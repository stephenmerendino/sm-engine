#pragma once

#include "engine/core/Types.h"
#include "engine/render/vulkan/VulkanInclude.h"

#include <vector>

class VulkanDevice;

class VulkanRenderPassAttachments
{
public:
	VulkanRenderPassAttachments();
	~VulkanRenderPassAttachments();

	void Add(VkFormat format, VkSampleCountFlagBits sampleCount,
			 VkImageLayout initialLayout, VkImageLayout finalLayout,
			 VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
			 VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			 VkAttachmentDescriptionFlags flags = 0);

	VkAttachmentDescription* GetAttachments();
	const std::vector<VkAttachmentDescription>& GetAttachments() const;
	U32 GetAttachmentCount() const;

private:
	std::vector<VkAttachmentDescription> m_vkAttachments;
};

class VulkanSubpass
{
public:
	VulkanSubpass();
	~VulkanSubpass();

	void AddColorAttachmentRef(U32 attachment, VkImageLayout layout);
	void AddResolveAttachmentRef(U32 attachment, VkImageLayout layout);
	void AddDepthStencilAttachmentRef(U32 attachment, VkImageLayout layout);
	VkSubpassDescription GetSubpassDescription() const;

private:
	std::vector<VkAttachmentReference> m_vkColorAttachmentRefs;
	std::vector<VkAttachmentReference> m_vkResolveAttachmentRefs;
	VkAttachmentReference m_vkDepthAttachmentRef;
	bool m_bHasDepthAttachment;
};

class VulkanRenderPass
{
public:
	VulkanRenderPass(VulkanDevice* pDevice);
	~VulkanRenderPass();

	void SetAttachments(const VulkanRenderPassAttachments& attachments);
	void AddSubpass(const VulkanSubpass& subpass);
	void AddSubpassDependency(U32 srcSubpass, U32 dstSubpass, VkPipelineStageFlags srcStageMask, VkAccessFlags srcAccessMask, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask);
	bool Setup();
	void Teardown();
	VkRenderPass GetHandle();
	VkFramebuffer CreateFrameBuffer(const std::vector<VkImageView> attachments, U32 width, U32 height, U32 layerCount);

private:
	VulkanDevice* m_pDevice;
	VulkanRenderPassAttachments m_passAttachments;
	std::vector<VkSubpassDescription> m_subpasses;
	std::vector<VkSubpassDependency> m_subpassDependencies;
	VkRenderPass m_vkRenderPass;
};
