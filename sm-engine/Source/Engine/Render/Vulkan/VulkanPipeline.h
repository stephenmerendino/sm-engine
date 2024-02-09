#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanRenderPass.h"
#include <vector>

class VulkanDevice;
class Mesh;

class VulkanShaderStages
{
public:
    struct ShaderInfo
    {
        const char* m_filepath;
        const char* m_entryName;
    };

    void Init(const ShaderInfo& vertexInfo, const ShaderInfo& fragmentInfo);
    void Init(const char* vertexFilepath, const char* vertexEntryName, const char* fragmentFilepath, const char* fragmentEntryName);
    void Destroy();

    std::vector<VkShaderModule> m_shaders;
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;
};

class VulkanPipelineLayout
{
public:
    VulkanPipelineLayout();

    void Init(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void Destroy();

	VkPipelineLayout m_layoutHandle;
};

class VulkanMeshPipelineInputInfo
{
public:
    VulkanMeshPipelineInputInfo();

    void Init(const Mesh* pMesh, bool primitiveRestartEnabled);

    // input assembly state
    VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyInfo;

    // vertex input state
    VkPipelineVertexInputStateCreateInfo m_vertexInputInfo;
    std::vector<VkVertexInputBindingDescription> m_vertexInputBindingDescs;
    std::vector<VkVertexInputAttributeDescription> m_vertexInputAttrDescs;
};

class VulkanPipelineState
{
public:
    VulkanPipelineState();

    void PreInitAddColorBlendAttachment(bool bBlendEnable = false, VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE, VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, VkBlendOp colorBlendOp = VK_BLEND_OP_ADD,
                                                                   VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD,
                                                                   VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

	void InitRasterState(VkPolygonMode polygonMode,
						 VkFrontFace frontFace,
						 VkCullModeFlags cullMode,
						 bool rasterDiscardEnable = false,
						 bool depthClampEnable = false,
						 bool depthBiasEnable = false,
						 F32 depthBiasConstant = 0.0f,
						 F32 depthBiasClamp = 0.0f,
						 F32 depthBiasSlope = 0.0f,
						 F32 lineWidth = 1.0f);

	void InitViewportState(F32 x, F32 y,
						   F32 w, F32 h,
						   F32 minDepth = 0.0f, F32 maxDepth = 1.0f,
						   I32 scissorOffsetX = 0, I32 scissorOffsetY = 0,
						   U32 scissorExtentX = 0, U32 scissorExtentY = 0);

    void InitMultisampleState(VkSampleCountFlagBits sampleCount, bool sampleShadingEnable = false, F32 minSampleShading = 0.0f);

    void InitDepthStencilState(bool depthTestEnable, bool depthWriteEnable, VkCompareOp depthCompareOp, 
                               bool depthBoundsTestEnable = false, F32 minDepthBounds = 0.0f, F32 maxDepthBounds = 1.0f);

    void InitColorBlendState(bool logicOpEnable, VkLogicOp logicOp = VK_LOGIC_OP_CLEAR, F32 blendConstant0 = 0.0f, F32 blendConstant1 = 0.0f, F32 blendConstant2 = 0.0f, F32 blendConstant3 = 0.0f);
    
    bool IsFullyInitialized() const;

    bool m_bDidInitRasterState : 1;
    bool m_bDidInitViewportState : 1;
    bool m_bDidInitMultisampleState : 1;
    bool m_bDidInitDepthStencilState : 1;
    bool m_bDidInitColorBlendState : 1;

    VkPipelineRasterizationStateCreateInfo m_rasterState = {};

    VkPipelineViewportStateCreateInfo m_viewportState = {};
    VkViewport m_viewport = {};
    VkRect2D m_scissor = {};

    VkPipelineMultisampleStateCreateInfo m_multisampleState = {};

    VkPipelineDepthStencilStateCreateInfo m_depthStencilState = {};

    VkPipelineColorBlendStateCreateInfo m_colorBlendState = {};
    std::vector<VkPipelineColorBlendAttachmentState> m_colorBlendAttachments;
};

class VulkanPipeline
{
public:
	VulkanPipeline();

    void Init(const VulkanShaderStages& shaderStages,
              const VulkanPipelineLayout& layout,
              const VulkanMeshPipelineInputInfo& meshPipelineInputInfo,
              const VulkanPipelineState& pipelineState,
              const VulkanRenderPass& renderPass);

    void Destroy();

    VkPipeline m_pipelineHandle;
};
