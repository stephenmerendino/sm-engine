#include "Engine/Render/Vulkan/VulkanPipeline.h"
#include "Engine/Config.h"
#include "Engine/Core/Types.h"
#include "Engine/Core/File.h"
#include "Engine/Render/Mesh.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Render/Vulkan/VulkanFormats.h"

VkShaderModule CreateShaderModule(const char* shaderFilepath)
{
	std::vector<Byte> rawShaderCode = ReadBinaryFile(shaderFilepath);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = rawShaderCode.size();
	createInfo.pCode = (const U32*)rawShaderCode.data();

	VkShaderModule shaderModule = VK_NULL_HANDLE;
	SM_VULKAN_ASSERT(vkCreateShaderModule(VulkanDevice::GetHandle(), &createInfo, nullptr, &shaderModule));

	return shaderModule;
}

void VulkanShaderStages::Init(const Shader& vertexShader, const Shader& fragmentShader)
{
	//Init(vertexInfo.m_filepath, vertexInfo.m_entryName, fragmentInfo.m_filepath, fragmentInfo.m_entryName);
}

void VulkanShaderStages::Init(const char* vertexFilepath, const char* vertexEntryName, const char* fragmentFilepath, const char* fragmentEntryName)
{
    std::string vertexFullFilepath = std::string(SHADERS_PATH) + std::string(vertexFilepath);
    std::string pipelineFullFilepath = std::string(SHADERS_PATH) + std::string(fragmentFilepath);

    VkShaderModule vertShader = CreateShaderModule(vertexFullFilepath.c_str());
    VkShaderModule fragShader = CreateShaderModule(pipelineFullFilepath.c_str());

    VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo = {};
    vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageCreateInfo.module = vertShader;
    vertShaderStageCreateInfo.pName = vertexEntryName;

    VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo = {};
    fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageCreateInfo.module = fragShader;
    fragShaderStageCreateInfo.pName = fragmentEntryName;

    m_shaders.push_back(vertShader);
    m_shaders.push_back(fragShader);
    m_shaderStageInfos.push_back(vertShaderStageCreateInfo);
    m_shaderStageInfos.push_back(fragShaderStageCreateInfo);
}

void VulkanShaderStages::Destroy()
{
    for (int i = 0; i < (int)m_shaders.size(); i++)
    {
        vkDestroyShaderModule(VulkanDevice::GetHandle(), m_shaders[i], nullptr);
    }
}

VulkanPipelineLayout::VulkanPipelineLayout()
    :m_layoutHandle(VK_NULL_HANDLE)
{
}

void VulkanPipelineLayout::Init(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = (U32)descriptorSetLayouts.size();
    layoutInfo.pSetLayouts = descriptorSetLayouts.data();
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = nullptr;

    SM_VULKAN_ASSERT(vkCreatePipelineLayout(VulkanDevice::GetHandle(), &layoutInfo, nullptr, &m_layoutHandle));
}

void VulkanPipelineLayout::Destroy()
{
    vkDestroyPipelineLayout(VulkanDevice::GetHandle(), m_layoutHandle, nullptr);
}

VulkanMeshPipelineInputInfo::VulkanMeshPipelineInputInfo()
    :m_inputAssemblyInfo({})
    ,m_vertexInputInfo({})
{
}

void VulkanMeshPipelineInputInfo::Init(const Mesh* pMesh, bool primitiveRestartEnabled)
{
	m_inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_inputAssemblyInfo.primitiveRestartEnable = primitiveRestartEnabled;
    m_inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // default to triangle list in case nullptr mesh is passed in (no vertex buffer draw)

    if (pMesh)
    {
        switch (pMesh->m_topology)
        {
            case PrimitiveTopology::kPointList: m_inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case PrimitiveTopology::kLineList: m_inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PrimitiveTopology::kTriangleList: m_inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }

        m_vertexInputBindingDescs = VulkanFormats::GetVertexInputBindingDescs(VertexType::kPCT);
        m_vertexInputAttrDescs = VulkanFormats::GetVertexInputAttrDescs(VertexType::kPCT);
    }

	m_vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_vertexInputInfo.vertexBindingDescriptionCount = (U32)m_vertexInputBindingDescs.size();
	m_vertexInputInfo.pVertexBindingDescriptions = m_vertexInputBindingDescs.data();
	m_vertexInputInfo.vertexAttributeDescriptionCount = (U32)m_vertexInputAttrDescs.size();
	m_vertexInputInfo.pVertexAttributeDescriptions = m_vertexInputAttrDescs.data();
}

VulkanPipelineState::VulkanPipelineState()
    :m_bDidInitRasterState(false)
    ,m_bDidInitViewportState(false)
    ,m_bDidInitMultisampleState(false)
    ,m_bDidInitDepthStencilState(false)
    ,m_bDidInitColorBlendState(false)
{
}

void VulkanPipelineState::PreInitAddColorBlendAttachment(bool bBlendEnable, VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor, VkBlendOp colorBlendOp, 
                                                                            VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor, VkBlendOp alphaBlendOp, 
                                                                            VkColorComponentFlags colorWriteMask)
{
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = bBlendEnable;
    colorBlendAttachment.colorWriteMask = colorWriteMask;
    colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
    colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
    colorBlendAttachment.colorBlendOp = colorBlendOp;
    colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
    colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
    colorBlendAttachment.alphaBlendOp = alphaBlendOp;
    m_colorBlendAttachments.push_back(colorBlendAttachment);
}

void VulkanPipelineState::InitRasterState(VkPolygonMode polygonMode,
                                          VkFrontFace frontFace,
                                          VkCullModeFlags cullMode,
                                          bool rasterDiscardEnable,
                                          bool depthClampEnable,
                                          bool depthBiasEnable,
                                          F32 depthBiasConstant,
                                          F32 depthBiasClamp,
                                          F32 depthBiasSlope,
                                          F32 lineWidth)
{
	m_rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_rasterState.depthClampEnable = depthClampEnable;
	m_rasterState.rasterizerDiscardEnable = rasterDiscardEnable;
	m_rasterState.polygonMode = polygonMode;
	m_rasterState.lineWidth = lineWidth;
	m_rasterState.cullMode = cullMode;
	m_rasterState.frontFace = frontFace;
	m_rasterState.depthBiasEnable = depthBiasEnable;
	m_rasterState.depthBiasConstantFactor = depthBiasConstant;
	m_rasterState.depthBiasClamp = depthBiasClamp;
	m_rasterState.depthBiasSlopeFactor = depthBiasSlope;

    m_bDidInitRasterState = true;
}

void VulkanPipelineState::InitViewportState(F32 x, F32 y,
                                            F32 w, F32 h,
                                            F32 minDepth, F32 maxDepth,
                                            I32 scissorOffsetX, I32 scissorOffsetY,
                                            U32 scissorExtentX, U32 scissorExtentY)
{
	m_viewport.x = x;
	m_viewport.y = y;
	m_viewport.width = w;
	m_viewport.height = h;
	m_viewport.minDepth = minDepth;
	m_viewport.maxDepth = maxDepth;

	m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_viewportState.pViewports = &m_viewport;
	m_viewportState.viewportCount = 1;
	m_viewportState.pScissors = nullptr;
	m_viewportState.scissorCount = 0;

    if (scissorExtentX > 0 && scissorExtentY > 0)
    {
        m_scissor.offset = { scissorOffsetX, scissorOffsetY };
        m_scissor.extent = { scissorExtentX, scissorExtentY };
        m_viewportState.pScissors = &m_scissor;
        m_viewportState.scissorCount = 1;
    }

    m_bDidInitViewportState = true;
}

void VulkanPipelineState::InitMultisampleState(VkSampleCountFlagBits sampleCount, bool sampleShadingEnable, F32 minSampleShading)
{
    m_multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_multisampleState.sampleShadingEnable = sampleShadingEnable;
    m_multisampleState.rasterizationSamples = sampleCount;
    m_multisampleState.minSampleShading = minSampleShading;
    m_multisampleState.pSampleMask = nullptr;
    m_multisampleState.alphaToCoverageEnable = VK_FALSE;
    m_multisampleState.alphaToOneEnable = VK_FALSE;

    m_bDidInitMultisampleState = true;
}

void VulkanPipelineState::InitDepthStencilState(bool depthTestEnable, bool depthWriteEnable, VkCompareOp depthCompareOp,
                                                bool depthBoundsTestEnable, F32 minDepthBounds, F32 maxDepthBounds)
{
    m_depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_depthStencilState.depthTestEnable = depthTestEnable;
    m_depthStencilState.depthWriteEnable = depthWriteEnable;
    m_depthStencilState.depthCompareOp = depthCompareOp;
    m_depthStencilState.depthBoundsTestEnable = depthBoundsTestEnable;
    m_depthStencilState.minDepthBounds = minDepthBounds;
    m_depthStencilState.maxDepthBounds = maxDepthBounds;
    //TODO Allow stencil to be passed in and set
    m_depthStencilState.stencilTestEnable = VK_FALSE;
    m_depthStencilState.front = {};
    m_depthStencilState.back = {};

    m_bDidInitDepthStencilState = true;
}

void VulkanPipelineState::InitColorBlendState(bool logicOpEnable, VkLogicOp logicOp, F32 blendConstant0, F32 blendConstant1, F32 blendConstant2, F32 blendConstant3)
{
    m_colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_colorBlendState.logicOpEnable = logicOpEnable;
    m_colorBlendState.logicOp = logicOp;
    m_colorBlendState.blendConstants[0] = blendConstant0;
    m_colorBlendState.blendConstants[1] = blendConstant1;
    m_colorBlendState.blendConstants[2] = blendConstant2;
    m_colorBlendState.blendConstants[3] = blendConstant3;
    m_colorBlendState.attachmentCount = (U32)m_colorBlendAttachments.size();
    m_colorBlendState.pAttachments = m_colorBlendAttachments.data();

    m_bDidInitColorBlendState = true;
}

bool VulkanPipelineState::IsFullyInitialized() const
{
    return m_bDidInitRasterState &&
           m_bDidInitViewportState &&
           m_bDidInitMultisampleState &&
           m_bDidInitDepthStencilState &&
           m_bDidInitColorBlendState;
}

VulkanPipeline::VulkanPipeline()
    :m_pipelineHandle(VK_NULL_HANDLE)
{
}

void VulkanPipeline::Init(const VulkanShaderStages& shaderStages,
                          const VulkanPipelineLayout& layout,
                          const VulkanMeshPipelineInputInfo& meshPipelineInputInfo,
                          const VulkanPipelineState& pipelineState,
                          const VulkanRenderPass& renderPass)
{
    SM_ASSERT(pipelineState.IsFullyInitialized());

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.stageCount = (U32)shaderStages.m_shaderStageInfos.size();
    pipelineCreateInfo.pStages = shaderStages.m_shaderStageInfos.data();
    pipelineCreateInfo.pVertexInputState = &meshPipelineInputInfo.m_vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &meshPipelineInputInfo.m_inputAssemblyInfo;
    pipelineCreateInfo.pRasterizationState = &pipelineState.m_rasterState;
    pipelineCreateInfo.pViewportState = &pipelineState.m_viewportState;
    pipelineCreateInfo.pMultisampleState = &pipelineState.m_multisampleState;
    pipelineCreateInfo.pDepthStencilState = &pipelineState.m_depthStencilState;
    pipelineCreateInfo.pColorBlendState = &pipelineState.m_colorBlendState;
    pipelineCreateInfo.pDynamicState = nullptr; //#TODO(smerendino): Enable this pipeline state
    pipelineCreateInfo.layout = layout.m_layoutHandle;
    pipelineCreateInfo.renderPass = renderPass.m_renderPassHandle;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    SM_VULKAN_ASSERT(vkCreateGraphicsPipelines(VulkanDevice::GetHandle(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipelineHandle));
}

void VulkanPipeline::Destroy()
{
	vkDestroyPipeline(VulkanDevice::GetHandle(), m_pipelineHandle, nullptr);
}
