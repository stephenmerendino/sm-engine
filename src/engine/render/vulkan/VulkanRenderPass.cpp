#include "Engine/Render/Vulkan/VulkanRenderPass.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Render/Vulkan/VulkanUtils.h"

#include "Engine/Core/Assert.h"

VulkanRenderPassAttachments::VulkanRenderPassAttachments()
{
}

VulkanRenderPassAttachments::~VulkanRenderPassAttachments()
{
}

void VulkanRenderPassAttachments::Add(VkFormat format, VkSampleCountFlagBits sampleCount,
									  VkImageLayout initialLayout, VkImageLayout finalLayout,
									  VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
									  VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp,
									  VkAttachmentDescriptionFlags flags)
{
	VkAttachmentDescription newAttachment = SetupAttachmentDescription(format, sampleCount, initialLayout, finalLayout, loadOp, storeOp, stencilLoadOp, stencilStoreOp, flags);
	m_vkAttachments.push_back(newAttachment);
}

VkAttachmentDescription* VulkanRenderPassAttachments::GetAttachments()
{
	return m_vkAttachments.data();
}

const std::vector<VkAttachmentDescription>& VulkanRenderPassAttachments::GetAttachments() const 
{ 
	return m_vkAttachments; 
}

U32 VulkanRenderPassAttachments::GetAttachmentCount() const
{
	return (U32)m_vkAttachments.size();
}

VulkanSubpass::VulkanSubpass()
	:m_bHasDepthAttachment(false)
	,m_vkDepthAttachmentRef({})
{
}

VulkanSubpass::~VulkanSubpass()
{
}

void VulkanSubpass::AddColorAttachmentRef(U32 attachment, VkImageLayout layout)
{
	VkAttachmentReference attachmentRef{};
	attachmentRef.attachment = attachment;
	attachmentRef.layout = layout;
	m_vkColorAttachmentRefs.push_back(attachmentRef);
}

void VulkanSubpass::AddResolveAttachmentRef(U32 attachment, VkImageLayout layout)
{
	VkAttachmentReference attachmentRef{};
	attachmentRef.attachment = attachment;
	attachmentRef.layout = layout;
	m_vkResolveAttachmentRefs.push_back(attachmentRef);
}

void VulkanSubpass::AddDepthStencilAttachmentRef(U32 attachment, VkImageLayout layout)
{
	ASSERT(!m_bHasDepthAttachment);
	m_vkDepthAttachmentRef.attachment = attachment;
	m_vkDepthAttachmentRef.layout = layout;
	m_bHasDepthAttachment = true;
}

VkSubpassDescription VulkanSubpass::GetSubpassDescription() const
{
	VkSubpassDescription subpassDesc{};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = (U32)m_vkColorAttachmentRefs.size();
	subpassDesc.pColorAttachments = m_vkColorAttachmentRefs.data();
	subpassDesc.pResolveAttachments = m_vkResolveAttachmentRefs.data();
	subpassDesc.pDepthStencilAttachment = &m_vkDepthAttachmentRef;
	return subpassDesc;
}

VulkanRenderPass::VulkanRenderPass(VulkanDevice* pDevice)
	:m_pDevice(pDevice)
	,m_vkRenderPass(VK_NULL_HANDLE)
{
}

VulkanRenderPass::~VulkanRenderPass()
{
}

void VulkanRenderPass::SetAttachments(const VulkanRenderPassAttachments& attachments)
{
	m_passAttachments = attachments;
}

void VulkanRenderPass::AddSubpass(const VulkanSubpass& subpass)
{
	m_subpasses.push_back(subpass.GetSubpassDescription());
}

void VulkanRenderPass::AddSubpassDependency(U32 srcSubpass, U32 dstSubpass, VkPipelineStageFlags srcStageMask, VkAccessFlags srcAccessMask, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask)
{
	VkSubpassDependency dependency{};
	dependency.srcSubpass = srcSubpass;
	dependency.dstSubpass = dstSubpass;
	dependency.srcStageMask = srcStageMask;
	dependency.srcAccessMask = srcAccessMask;
	dependency.dstStageMask = dstStageMask;
	dependency.dstAccessMask = dstAccessMask;
	m_subpassDependencies.push_back(dependency);
}

bool VulkanRenderPass::Setup()
{
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = m_passAttachments.GetAttachmentCount();
	renderPassInfo.pAttachments = m_passAttachments.GetAttachments();
	renderPassInfo.subpassCount = (U32)m_subpasses.size();
	renderPassInfo.pSubpasses = m_subpasses.data();
	renderPassInfo.dependencyCount = (U32)m_subpassDependencies.size();
	renderPassInfo.pDependencies = m_subpassDependencies.data();

	VULKAN_ASSERT(vkCreateRenderPass(m_pDevice->GetHandle(), &renderPassInfo, nullptr, &m_vkRenderPass));
	return true;
}

void VulkanRenderPass::Teardown()
{
	vkDestroyRenderPass(m_pDevice->GetHandle(), m_vkRenderPass, nullptr);
}

VkRenderPass VulkanRenderPass::GetHandle()
{
	return m_vkRenderPass;
}

VkFramebuffer VulkanRenderPass::CreateFrameBuffer(const std::vector<VkImageView> attachments, U32 width, U32 height, U32 layerCount)
{
	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = m_vkRenderPass;
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.attachmentCount = (U32)attachments.size();
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = layerCount;

	VkFramebuffer framebuffer;
	VULKAN_ASSERT(vkCreateFramebuffer(m_pDevice->GetHandle(), &framebufferInfo, nullptr, &framebuffer));
	return framebuffer;

}