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

class VulkanMeshPipelineInputInfo
{
public:
    void Init(const Mesh* pMesh, bool primitiveRestartEnabled);

    // input assembly state
    VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyInfo;

    // vertex input state
    VkPipelineVertexInputStateCreateInfo m_vertexInputInfo;
    std::vector<VkVertexInputBindingDescription> m_vertexInputBindingDescs;
    std::vector<VkVertexInputAttributeDescription> m_vertexInputAttrDescs;
};

//class VulkanPipelineState
//{
//public:
//    VulkanPipelineState();
//
//    void InitRasterState();
//    void InitViewportState();
//    void InitMultisampleState();
//    void InitDepthStencilState();
//    void PreInitAddColorBlendAttachment();
//    void InitColorBlendState();
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
//  VkPipelineLayout m_pipelineLayout;
//	VkPipeline m_pipelineHandle;
//};
