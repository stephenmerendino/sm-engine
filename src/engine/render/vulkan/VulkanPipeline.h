#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Core/Types.h"

class VulkanDevice;
class VulkanRenderPass;
class RenderableMesh;

#include <vector>

class VulkanPipeline
{
public:
	VulkanPipeline(VulkanDevice* pDevice);
	~VulkanPipeline();

	bool Setup(VkPipeline vkPipeline, VkPipelineLayout vkPipelineLayout);
	void Teardown();

	VkPipeline GetPipelineHandle();
	VkPipelineLayout GetPipelineLayoutHandle();

private:
	VulkanDevice* m_pDevice;
	VkPipeline m_vkPipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_vkPipelineLayout = VK_NULL_HANDLE;
};

class VulkanPipelineColorBlendAttachments 
{
public:
	VulkanPipelineColorBlendAttachments();
	~VulkanPipelineColorBlendAttachments();

	void Reset();

	void Add(VkColorComponentFlags colorWriteMask, bool blendEnable,
			 VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor, VkBlendOp colorBlendOp,
			 VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor, VkBlendOp alphaBlendOp);
	std::vector<VkPipelineColorBlendAttachmentState> GetAttachments() const;
	U32 GetNumAttachments() const;

private:
	std::vector<VkPipelineColorBlendAttachmentState> m_vkColorBlendAttachments;
};

class VulkanPipelineFactory
{
public:
	VulkanPipelineFactory(VulkanDevice* pDevice);
	~VulkanPipelineFactory();

	void Reset();

	void SetVsPs(const char* vsFilepath, const char* vsEntry, const char* psFilepath, const char* psEntry);
	void SetVertexInput(const RenderableMesh* mesh, VkPrimitiveTopology topology);
	void SetViewportAndScissor(U32 viewportX, U32 viewportY, U32 width, U32 height, F32 minDepth, F32 maxDepth, I32 scissorOffsetX, I32 scissorOffsetY, U32 scissorWidth, U32 scissorHeight);
	void SetViewportAndScissor(U32 width, U32 height);
	void SetRasterizer(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace, bool depthClampEnable = false, bool rasterizerDiscardEnable = false,
					   F32 lineWidth = 1.0f, bool depthBiasEnable = false, F32 depthBiasConstantFactor = 0.0f, F32 depthBiasClamp = 0.0f, F32 depthBiasSlopeFactor = 0.0f);
	void SetMultisampling(VkSampleCountFlagBits sampleCount, bool sampleShadingEnable = false, F32 minSampleShading = 1.0f, bool alphaToCoverageEnable = false, bool alphaToOneEnable = false);
	void SetDepthStencil(bool depthTestEnable, bool depthWriteEnable, VkCompareOp compareOp, bool depthBoundsTestEnable, F32 minDepthBounds, F32 maxDepthBounds);
	void SetBlend(const VulkanPipelineColorBlendAttachments& colorBlendAttachments, bool logicOpEnable, VkLogicOp logicOp, F32 blendConstant0, F32 blendConstant1, F32 blendConstant2, F32 blendConstant3);
	void SetDynamicState();
	void SetPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
	void SetPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, const std::vector<VkPushConstantRange> pushConstants);
	VulkanPipeline* CreatePipeline(VulkanRenderPass* pRenderPass, U32 subpass);

private:
	VulkanDevice* m_pDevice;
	const RenderableMesh* m_pRenderableMesh;
	VulkanPipelineColorBlendAttachments m_colorBlendAttachments;
	VkViewport m_viewport;
	VkRect2D m_scissor;
	std::vector<VkShaderModule> m_loadedShaders;
	std::vector<VkPipelineShaderStageCreateInfo> m_shaderStagesCreateInfo;
	VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo m_rasterizerInfo;
	VkPipelineMultisampleStateCreateInfo m_multisampleInfo;
	VkPipelineDepthStencilStateCreateInfo m_depthStencilInfo;
	VkPipelineColorBlendStateCreateInfo m_colorBlendInfo;
	VkPipelineLayoutCreateInfo m_pipelineLayoutInfo;
};