#include "Engine/Render/Vulkan/VulkanRenderPass.h"
#include "Engine/Core/Assert.h"

void VulkanRenderPass::PreInitAddAttachment(VkFormat format, VkSampleCountFlagBits sampleCount,
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
	m_attachments.push_back(attachmentDesc);
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
	//render_pass_t render_pass;

	//std::vector<VkSubpassDescription2> subpass_descriptions;
	//subpass_descriptions.resize(subpasses.subpasses.size());

	//for (i32 i = 0; i < subpasses.subpasses.size(); i++)
	//{
	//	subpass_t& subpass = subpasses.subpasses[i];

	//	VkSubpassDescription2 subpass_desc = {};
	//	subpass_desc.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
	//	subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	//	subpass_desc.colorAttachmentCount = (u32)subpass.color_attach_refs.size();
	//	subpass_desc.pColorAttachments = subpass.color_attach_refs.data();
	//	subpass_desc.pResolveAttachments = subpass.color_resolve_attach_refs.data();
	//	subpass_desc.pDepthStencilAttachment = subpass.has_depth_attach ? &subpass.depth_attach_ref : VK_NULL_HANDLE;

	//	// resolve multisample depth if its needed using pNext w/ struct 
	//	VkSubpassDescriptionDepthStencilResolve ds_resolve = {};
	//	if (subpass.resolve_depth)
	//	{
	//		ds_resolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
	//		ds_resolve.depthResolveMode = VK_RESOLVE_MODE_MAX_BIT;
	//		ds_resolve.pDepthStencilResolveAttachment = &subpass.depth_resolve_attach_ref;
	//		subpass_desc.pNext = &ds_resolve;
	//	}

	//	subpass_descriptions[i] = subpass_desc;
	//}

	//VkRenderPassCreateInfo2 create_info = {};
	//create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
	//create_info.attachmentCount = (u32)attachments.attachments.size();
	//create_info.pAttachments = attachments.attachments.data();
	//create_info.subpassCount = (u32)subpass_descriptions.size();
	//create_info.pSubpasses = subpass_descriptions.data();
	//create_info.dependencyCount = (u32)subpass_dependencies.dependencies.size();
	//create_info.pDependencies = subpass_dependencies.dependencies.data();

	//SM_VULKAN_ASSERT(vkCreateRenderPass2(context.device.device_handle, &create_info, nullptr, &render_pass.handle));

	//render_pass.attachments = attachments;
	//render_pass.subpasses = subpasses;
	//render_pass.subpass_dependencies = subpass_dependencies;

	//return render_pass;
}

void VulkanRenderPass::Destroy()
{
	//vkDestroyRenderPass(context.device.device_handle, render_pass.handle, nullptr);
}
