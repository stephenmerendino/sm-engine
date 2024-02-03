#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanRenderPass.h"
#include <vector>

class VulkanDevice;

class VulkanShaderStages
{
public:
    struct ShaderInfo
    {
        const char* m_filepath;
        const char* m_entryName;
    };

    VulkanShaderStages();

    void Init(const VulkanDevice* pDevice, const ShaderInfo& vertexInfo, const ShaderInfo& fragmentInfo);
    void Init(const VulkanDevice* pDevice, const char* vertexFilepath, const char* vertexEntryName, const char* fragmentFilepath, const char* fragmentEntryName);
    void Destroy();

    const VulkanDevice* m_pDevice;
    std::vector<VkShaderModule> m_shaders;
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;
};

class VulkanPipelineLayout
{
public:
    VulkanPipelineLayout();

    void Init(const VulkanDevice* pDevice, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void Destroy();

    const VulkanDevice* m_pDevice;
	VkPipelineLayout m_layoutHandle;
};

//class VulkanMeshPipelineInputInfo
//{
//public:
//    // input assembly state
//    VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyInfo;
//
//    // vertex input state
//    VkPipelineVertexInputStateCreateInfo m_vertexInputInfo;
//    std::vector<VkVertexInputBindingDescription> m_inputBindingDescs;
//    std::vector<VkVertexInputAttributeDescription> m_inputAttrDescs;
//};
//
//class VulkanPipelineState
//{
//public:
//    VulkanPipelineState();
//
//    void SetRasterState();
//    void SetViewportState();
//    void SetMultisampleState();
//    void SetDepthStencilState();
//    void SetColorBlendState();
//
//    VkPipelineRasterizationStateCreateInfo m_rasterState;
//
//    VkPipelineViewportStateCreateInfo m_viewportState;
//    VkViewport m_viewport;
//    VkRect2D m_scissor;
//
//    VkPipelineMultisampleStateCreateInfo m_multisampleState;
//
//    VkPipelineDepthStencilStateCreateInfo m_depthStencilState;
//
//    VkPipelineColorBlendStateCreateInfo m_colorBlendState;
//    std::vector<VkPipelineColorBlendAttachmentState> m_colorBlendAttachments;
//};
//
//class VulkanPipeline
//{
//public:
//	VulkanPipeline();
//
//    void Init(const VulkanShaderStages& shaderStages,
//        const VulkanMeshPipelineInputInfo& meshPipelineInputInfo,
//        const VulkanPipelineState& pipelineState,
//        const VulkanPipelineLayout& layout,
//        const VulkanRenderPass& renderPass);
//
//	VkPipeline m_pipelineHandle;
//};
//
//class VulkanPipelineFactory
//{
//public:
//};
