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

    void PreInitAddColorBlendAttachment(bool bBlendEnable, VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor, VkBlendOp colorBlendOp,
                                                           VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor, VkBlendOp alphaBlendOp,
                                                           VkColorComponentFlags colorWriteMask);

	void InitRasterState(VkPolygonMode polygonMode,
						 VkFrontFace frontFace,
						 VkCullModeFlags cullMode,
						 bool rasterDiscardEnable,
						 bool depthClampEnable,
						 bool depthBiasEnable,
						 F32 depthBiasConstant,
						 F32 depthBiasClamp,
						 F32 depthBiasSlope,
						 F32 lineWidth);

	void InitViewportState(F32 x, F32 y,
						   F32 w, F32 h,
						   F32 minDepth, F32 maxDepth,
						   I32 scissorOffsetX, I32 scissorOffsetY,
						   U32 scissorExtentX, U32 scissorExtentY);

    void InitMultisampleState(VkSampleCountFlagBits sampleCount, bool sampleShadingEnable, F32 minSampleShading);

    void InitDepthStencilState(bool depthTestEnable, bool depthWriteEnable, VkCompareOp depthCompareOp, 
                               bool depthBoundsTestEnable, F32 minDepthBounds, F32 maxDepthBounds);

    void InitColorBlendState(bool logicOpEnable, VkLogicOp logicOp, F32 blendConstant0, F32 blendConstant1, F32 blendConstant2, F32 blendConstant3);
    
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
