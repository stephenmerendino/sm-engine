#include "engine/render/vulkan/VulkanPipeline.h"
#include "engine/render/vulkan/VulkanDevice.h"
#include "engine/render/vulkan/VulkanRenderPass.h"
#include "engine/render/RenderableMesh.h"
#include "engine/core/Assert.h"
#include "engine/core/Common.h"

VulkanPipeline::VulkanPipeline(VulkanDevice* pDevice)
	:m_pDevice(pDevice)
	,m_vkPipeline(VK_NULL_HANDLE)
	,m_vkPipelineLayout(VK_NULL_HANDLE)
{
}

VulkanPipeline::~VulkanPipeline()
{
}

bool VulkanPipeline::Setup(VkPipeline vkPipeline, VkPipelineLayout vkPipelineLayout)
{
	m_vkPipeline = vkPipeline;
	m_vkPipelineLayout = vkPipelineLayout;
	return true;
}

void VulkanPipeline::Teardown()
{
	vkDestroyPipeline(m_pDevice->GetHandle(), m_vkPipeline, nullptr);
	vkDestroyPipelineLayout(m_pDevice->GetHandle(), m_vkPipelineLayout, nullptr);
}

VkPipeline VulkanPipeline::GetPipelineHandle()
{
	return m_vkPipeline;
}

VkPipelineLayout VulkanPipeline::GetPipelineLayoutHandle()
{
	return m_vkPipelineLayout;
}

VulkanPipelineColorBlendAttachments::VulkanPipelineColorBlendAttachments()
{
}

VulkanPipelineColorBlendAttachments::~VulkanPipelineColorBlendAttachments()
{
}

void VulkanPipelineColorBlendAttachments::Reset()
{
	m_vkColorBlendAttachments.clear();
}

void VulkanPipelineColorBlendAttachments::Add(VkColorComponentFlags colorWriteMask, bool blendEnable, 
			VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor, VkBlendOp colorBlendOp, 
			VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor, VkBlendOp alphaBlendOp)
{
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = colorWriteMask;
	colorBlendAttachment.blendEnable = blendEnable;
	colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
	colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
	colorBlendAttachment.colorBlendOp = colorBlendOp;
	colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
	colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
	colorBlendAttachment.alphaBlendOp = alphaBlendOp;
	m_vkColorBlendAttachments.push_back(colorBlendAttachment);
}

std::vector<VkPipelineColorBlendAttachmentState> VulkanPipelineColorBlendAttachments::GetAttachments() const
{
	return m_vkColorBlendAttachments;
}

U32 VulkanPipelineColorBlendAttachments::GetNumAttachments() const
{
	return (U32)m_vkColorBlendAttachments.size();
}

VulkanPipelineFactory::VulkanPipelineFactory(VulkanDevice* pDevice)
	:m_pDevice(pDevice)
	,m_pRenderableMesh(nullptr)
	,m_viewport({})
	,m_scissor({})
	,m_inputAssemblyInfo({})
	,m_rasterizerInfo({})
	,m_multisampleInfo({})
	,m_depthStencilInfo({})
	,m_colorBlendInfo({})
	,m_pipelineLayoutInfo({})
{
}

VulkanPipelineFactory::~VulkanPipelineFactory()
{
}

void VulkanPipelineFactory::Reset()
{
	m_pRenderableMesh = nullptr;
	MemZero(m_colorBlendAttachments);
	MemZero(m_viewport);
	MemZero(m_scissor);
	m_loadedShaders.clear();
	m_shaderStagesCreateInfo.clear();
	MemZero(m_inputAssemblyInfo);
	MemZero(m_rasterizerInfo);
	MemZero(m_multisampleInfo);
	MemZero(m_depthStencilInfo);
	MemZero(m_colorBlendInfo);
	MemZero(m_pipelineLayoutInfo);
}

void VulkanPipelineFactory::SetVsPs(const char* vsFilepath, const char* vsEntry, const char* psFilepath, const char* psEntry)
{
	VkShaderModule vertShader = m_pDevice->CreateShaderModule(vsFilepath);
	VkShaderModule fragShader = m_pDevice->CreateShaderModule(psFilepath);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShader;
	vertShaderStageInfo.pName = vsEntry;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShader;
	fragShaderStageInfo.pName = psEntry;

	m_loadedShaders.push_back(vertShader);
	m_loadedShaders.push_back(fragShader);
	m_shaderStagesCreateInfo.push_back(vertShaderStageInfo);
	m_shaderStagesCreateInfo.push_back(fragShaderStageInfo);
}

void VulkanPipelineFactory::SetVertexInput(const RenderableMesh* mesh, VkPrimitiveTopology topology)
{
	m_pRenderableMesh = mesh;

	// input assembly
	m_inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_inputAssemblyInfo.topology = topology;
	m_inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
}

void VulkanPipelineFactory::SetViewportAndScissor(U32 viewportX, U32 viewportY, U32 width, U32 height, F32 minDepth, F32 maxDepth, I32 scissorOffsetX, I32 scissorOffsetY, U32 scissorWidth, U32 scissorHeight)
{
	// viewports and scissors
	m_viewport.x = (F32)viewportX;
	m_viewport.y = (F32)viewportY;
	m_viewport.width = (F32)width;
	m_viewport.height = (F32)height;
	m_viewport.minDepth = minDepth;
	m_viewport.maxDepth = maxDepth;

	m_scissor.offset = { scissorOffsetX, scissorOffsetY };
	m_scissor.extent = { scissorWidth, scissorHeight};
}

void VulkanPipelineFactory::SetViewportAndScissor(U32 width, U32 height)
{
	SetViewportAndScissor(0, 0, width, height, 0.0f, 1.0f, 0, 0, width, height);
}

void VulkanPipelineFactory::SetRasterizer(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace, bool depthClampEnable, bool rasterizerDiscardEnable, 
						F32 lineWidth, bool depthBiasEnable, F32 depthBiasConstantFactor, F32 depthBiasClamp, F32 depthBiasSlopeFactor)
{
	m_rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_rasterizerInfo.depthClampEnable = depthClampEnable;
	m_rasterizerInfo.rasterizerDiscardEnable = rasterizerDiscardEnable;
	m_rasterizerInfo.polygonMode = polygonMode;
	m_rasterizerInfo.lineWidth = lineWidth;
	m_rasterizerInfo.cullMode = cullMode;
	m_rasterizerInfo.frontFace = frontFace;
	m_rasterizerInfo.depthBiasEnable = depthBiasEnable;
	m_rasterizerInfo.depthBiasConstantFactor = depthBiasConstantFactor;
	m_rasterizerInfo.depthBiasClamp = depthBiasClamp;
	m_rasterizerInfo.depthBiasSlopeFactor = depthBiasSlopeFactor;
}

void VulkanPipelineFactory::SetMultisampling(VkSampleCountFlagBits sampleCount, bool sampleShadingEnable, F32 minSampleShading, bool alphaToCoverageEnable, bool alphaToOneEnable)
{
	m_multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_multisampleInfo.sampleShadingEnable = sampleShadingEnable;
	m_multisampleInfo.rasterizationSamples = sampleCount;
	m_multisampleInfo.minSampleShading = minSampleShading;
	m_multisampleInfo.pSampleMask = nullptr;
	m_multisampleInfo.alphaToCoverageEnable = alphaToCoverageEnable;
	m_multisampleInfo.alphaToOneEnable = alphaToOneEnable;
}

void VulkanPipelineFactory::SetDepthStencil(bool depthTestEnable, bool depthWriteEnable, VkCompareOp compareOp, bool depthBoundsTestEnable, F32 minDepthBounds, F32 maxDepthBounds)
{
	m_depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	m_depthStencilInfo.depthTestEnable = depthTestEnable;
	m_depthStencilInfo.depthWriteEnable = depthWriteEnable;
	m_depthStencilInfo.depthCompareOp = compareOp;
	m_depthStencilInfo.depthBoundsTestEnable = depthBoundsTestEnable;
	m_depthStencilInfo.minDepthBounds = minDepthBounds;
	m_depthStencilInfo.maxDepthBounds = maxDepthBounds;

	//#TODO Need to be able to set stencil state through args
	m_depthStencilInfo.stencilTestEnable = VK_FALSE;
	m_depthStencilInfo.front = {};
	m_depthStencilInfo.back = {};
}

void VulkanPipelineFactory::SetBlend(const VulkanPipelineColorBlendAttachments& colorBlendAttachments, bool logicOpEnable, VkLogicOp logicOp, F32 blendConstant0, F32 blendConstant1, F32 blendConstant2, F32 blendConstant3)
{
	m_colorBlendAttachments = colorBlendAttachments; // save this off to member variable so that pAttachments is fine

	m_colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_colorBlendInfo.logicOpEnable = logicOpEnable;
	m_colorBlendInfo.logicOp = logicOp;
	m_colorBlendInfo.blendConstants[0] = blendConstant0;
	m_colorBlendInfo.blendConstants[1] = blendConstant1;
	m_colorBlendInfo.blendConstants[2] = blendConstant2;
	m_colorBlendInfo.blendConstants[3] = blendConstant3;
}

void VulkanPipelineFactory::SetDynamicState()
{
	//#TODO Add this to pipeline creation
}

void VulkanPipelineFactory::SetPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
	m_pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	m_pipelineLayoutInfo.setLayoutCount = (U32)descriptorSetLayouts.size();
	m_pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	m_pipelineLayoutInfo.pushConstantRangeCount = 0;
	m_pipelineLayoutInfo.pPushConstantRanges = nullptr;
}

void VulkanPipelineFactory::SetPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, const std::vector<VkPushConstantRange> pushConstants)
{
	m_pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	m_pipelineLayoutInfo.setLayoutCount = (U32)descriptorSetLayouts.size();
	m_pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	m_pipelineLayoutInfo.pushConstantRangeCount = (U32)pushConstants.size();
	m_pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
}

VulkanPipeline* VulkanPipelineFactory::CreatePipeline(VulkanRenderPass* pRenderPass, U32 subpass)
{
	VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
	VULKAN_ASSERT(vkCreatePipelineLayout(m_pDevice->GetHandle(), &m_pipelineLayoutInfo, nullptr, &vkPipelineLayout));

	std::vector<VkVertexInputBindingDescription> inputBindingDesc = m_pRenderableMesh->GetVertexInputBindingDesc();
	std::vector<VkVertexInputAttributeDescription> inputAttrDesc = m_pRenderableMesh->GetVertexAttrDescs();

	// vertex input
	VkPipelineVertexInputStateCreateInfo vertexInputStateInfo{};
	vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateInfo.vertexBindingDescriptionCount = (U32)inputBindingDesc.size();
	vertexInputStateInfo.pVertexBindingDescriptions = inputBindingDesc.data();
	vertexInputStateInfo.vertexAttributeDescriptionCount = (U32)(inputAttrDesc.size());
	vertexInputStateInfo.pVertexAttributeDescriptions = inputAttrDesc.data();

	// viewport
	VkPipelineViewportStateCreateInfo viewportStateInfo{};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.pViewports = &m_viewport;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.pScissors = &m_scissor;
	viewportStateInfo.scissorCount = 1;

	// color blend
	m_colorBlendInfo.attachmentCount = m_colorBlendAttachments.GetNumAttachments();
	std::vector<VkPipelineColorBlendAttachmentState> attachments = m_colorBlendAttachments.GetAttachments();
	m_colorBlendInfo.pAttachments = attachments.data();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = (U32)m_shaderStagesCreateInfo.size();
	pipelineInfo.pStages = m_shaderStagesCreateInfo.data();
	pipelineInfo.pVertexInputState = &vertexInputStateInfo;
	pipelineInfo.pInputAssemblyState = &m_inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pRasterizationState = &m_rasterizerInfo;
	pipelineInfo.pMultisampleState = &m_multisampleInfo;
	pipelineInfo.pDepthStencilState = &m_depthStencilInfo;
	pipelineInfo.pColorBlendState = &m_colorBlendInfo;
	pipelineInfo.pDynamicState = nullptr; //#TODO Enable this pipeline state
	pipelineInfo.layout = vkPipelineLayout;
	pipelineInfo.renderPass = pRenderPass->GetHandle();
	pipelineInfo.subpass = subpass;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	VkPipeline vkPipeline = VK_NULL_HANDLE;
	VULKAN_ASSERT(vkCreateGraphicsPipelines(m_pDevice->GetHandle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipeline));

	// cleanup
	for (VkShaderModule shaderModule : m_loadedShaders)
	{
		vkDestroyShaderModule(m_pDevice->GetHandle(), shaderModule, nullptr);
	}

	VulkanPipeline* pipeline = new VulkanPipeline(m_pDevice);
	pipeline->Setup(vkPipeline, vkPipelineLayout);
	return pipeline;
}
