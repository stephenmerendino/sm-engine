#include "Engine/Render/Vulkan/VulkanRenderPass.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Core/Assert_old.h"

VulkanRenderPass::VulkanRenderPass()
	:m_renderPassHandle(VK_NULL_HANDLE)
{
}

void VulkanRenderPass::PreInitAddAttachmentDesc(VkFormat format, VkSampleCountFlagBits sampleCount,
											VkImageLayout initialLayout, VkImageLayout finalLayout,
											VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
											VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp,
											VkAttachmentDescriptionFlags flags)
{
	VkAttachmentDescription2 attachmentDesc = {};
	attachmentDesc.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	attachmentDesc.format = format;
	attachmentDesc.samples = sampleCount;
	attachmentDesc.loadOp = loadOp;
	attachmentDesc.storeOp = storeOp;
	attachmentDesc.stencilLoadOp = stencilLoadOp;
	attachmentDesc.stencilStoreOp = stencilStoreOp;
	attachmentDesc.initialLayout = initialLayout;
	attachmentDesc.finalLayout = finalLayout;
	attachmentDesc.flags = flags;
	m_attachmentDescs.push_back(attachmentDesc);
}

void VulkanRenderPass::PreInitAddSubpassAttachmentReference(U32 subpassIndex, VulkanSubpass::AttachmentRefType type, U32 attachment, VkImageLayout layout)
{
	VkAttachmentReference2 attachRef = {};
	attachRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	attachRef.attachment = attachment;
	attachRef.layout = layout;

	if (subpassIndex >= m_subpasses.size())
	{
		m_subpasses.resize(subpassIndex + 1);
	}

	VulkanSubpass& subpass = m_subpasses[subpassIndex];

	switch (type)
	{
		case VulkanSubpass::AttachmentRefType::COLOR:
			subpass.m_colorAttachRefs.push_back(attachRef);
			break;
		case VulkanSubpass::AttachmentRefType::DEPTH:
			SM_ASSERT(subpass.m_bHasDepthAttach == false);
			subpass.m_depthAttachRef = attachRef;
			subpass.m_bHasDepthAttach = true;
			break;
		case VulkanSubpass::AttachmentRefType::COLOR_RESOLVE:
			subpass.m_colorResolveAttachRefs.push_back(attachRef);
			break;
		case VulkanSubpass::AttachmentRefType::DEPTH_RESOLVE:
			SM_ASSERT(subpass.m_bHasDepthResolveAttach == false);
			subpass.m_depthResolveAttachRef = attachRef;
			subpass.m_bHasDepthResolveAttach = true;
			break;
	}
}

void VulkanRenderPass::PreInitAddSubpassDependency(U32 srcSubpass,
												   U32 dstSubpass,
												   VkPipelineStageFlags srcStage,
												   VkPipelineStageFlags dstStage,
												   VkAccessFlags srcAccess,
												   VkAccessFlags dstAccess,
												   VkDependencyFlags dependencyFlags)
{
	VkSubpassDependency2 subpassDependency = {};
	subpassDependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
	subpassDependency.srcSubpass = srcSubpass;
	subpassDependency.dstSubpass = dstSubpass;
	subpassDependency.srcStageMask = srcStage;
	subpassDependency.dstStageMask = dstStage;
	subpassDependency.srcAccessMask = srcAccess;
	subpassDependency.dstAccessMask = dstAccess;
	subpassDependency.dependencyFlags = dependencyFlags;
	m_subpassDependencies.push_back(subpassDependency);
}

void VulkanRenderPass::Init()
{
	std::vector<VkSubpassDescription2> subpassDescriptions;
	subpassDescriptions.resize(m_subpasses.size());

	for (I32 i = 0; i < (I32)m_subpasses.size(); i++)
	{
		VulkanSubpass& subpass = m_subpasses[i];

		VkSubpassDescription2 subpassDesc = {};
		subpassDesc.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDesc.colorAttachmentCount = (U32)subpass.m_colorAttachRefs.size();
		subpassDesc.pColorAttachments = subpass.m_colorAttachRefs.data();
		subpassDesc.pResolveAttachments = subpass.m_colorResolveAttachRefs.data();
		subpassDesc.pDepthStencilAttachment = subpass.m_bHasDepthAttach ? &subpass.m_depthAttachRef : VK_NULL_HANDLE;

		// resolve multisample depth if its needed using pNext w/ struct 
		VkSubpassDescriptionDepthStencilResolve depthStencilResolve = {};
		if (subpass.m_bHasDepthResolveAttach)
		{
			depthStencilResolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
			depthStencilResolve.depthResolveMode = VK_RESOLVE_MODE_MAX_BIT;
			depthStencilResolve.pDepthStencilResolveAttachment = &subpass.m_depthResolveAttachRef;
			subpassDesc.pNext = &depthStencilResolve;
		}

		subpassDescriptions[i] = subpassDesc;
	}

	VkRenderPassCreateInfo2 createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
	createInfo.attachmentCount = (U32)m_attachmentDescs.size();
	createInfo.pAttachments = m_attachmentDescs.data();
	createInfo.subpassCount = (U32)subpassDescriptions.size();
	createInfo.pSubpasses = subpassDescriptions.data();
	createInfo.dependencyCount = (U32)m_subpassDependencies.size();
	createInfo.pDependencies = m_subpassDependencies.data();

	SM_VULKAN_ASSERT(vkCreateRenderPass2(VulkanDevice::GetHandle(), &createInfo, nullptr, &m_renderPassHandle));
}

void VulkanRenderPass::Destroy()
{
	vkDestroyRenderPass(VulkanDevice::GetHandle(), m_renderPassHandle, nullptr);
}
