#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"
#include <vector>

struct VulkanSubpass
{
	std::vector<VkAttachmentReference2> m_colorAttachRefs;
	std::vector<VkAttachmentReference2> m_colorResolveAttachRefs;
	VkAttachmentReference2 m_depthAttachRef = {};
	VkAttachmentReference2 m_depthResolveAttachRef = {};
	bool m_bHasDepthAttach = false;
	bool m_bHasDepthResolveAttach = false;

	enum AttachmentRefType : U8
	{
		COLOR,
		DEPTH,
		COLOR_RESOLVE,
		DEPTH_RESOLVE
	};
};

class VulkanRenderPass
{
public:
	void PreInitAddAttachment(VkFormat format, VkSampleCountFlagBits sampleCount,
							  VkImageLayout initialLayout, VkImageLayout finalLayout,
							  VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
							  VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp,
							  VkAttachmentDescriptionFlags flags);
	void PreInitAddSubpassAttachmentReference(U32 subpassIndex, VulkanSubpass::AttachmentRefType type, U32 attachment, VkImageLayout layout);
	void PreInitAddSubpassDependency(U32 srcSubpass,
									 U32 dstSubpass,
									 VkPipelineStageFlags srcStage,
									 VkPipelineStageFlags dstStage,
									 VkAccessFlags srcAccess,
									 VkAccessFlags dstAccess,
									 VkDependencyFlags dependencyFlags = 0);
	void Init();
	void Destroy();

	std::vector<VkAttachmentDescription2> m_attachments;
	std::vector<VulkanSubpass> m_subpasses;
	std::vector<VkSubpassDependency2> m_subpassDependencies;
};
