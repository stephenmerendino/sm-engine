#include "Engine/Render/Vulkan/VulkanPipeline.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Render/Vulkan/VulkanFormats.h"
#include "Engine/Core/Types.h"
#include "Engine/Core/File.h"
#include "Engine/Render/Mesh.h"

VkShaderModule CreateShaderModule(const VulkanDevice* pDevice, const char* shaderFilepath)
{
	std::vector<Byte> rawShaderCode = ReadBinaryFile(shaderFilepath);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = rawShaderCode.size();
	createInfo.pCode = (const U32*)rawShaderCode.data();

	VkShaderModule shaderModule = VK_NULL_HANDLE;
	SM_VULKAN_ASSERT(vkCreateShaderModule(pDevice->m_deviceHandle, &createInfo, nullptr, &shaderModule));

	return shaderModule;
}

VulkanShaderStages::VulkanShaderStages()
	:m_pDevice(nullptr)
{
}

void VulkanShaderStages::Init(const VulkanDevice* pDevice, const ShaderInfo& vertexInfo, const ShaderInfo& fragmentInfo)
{
	Init(pDevice, vertexInfo.m_filepath, vertexInfo.m_entryName, fragmentInfo.m_filepath, fragmentInfo.m_entryName);
}

void VulkanShaderStages::Init(const VulkanDevice* pDevice, const char* vertexFilepath, const char* vertexEntryName, const char* fragmentFilepath, const char* fragmentEntryName)
{
	m_pDevice = pDevice;

    VkShaderModule vertShader = CreateShaderModule(pDevice, vertexFilepath);
    VkShaderModule fragShader = CreateShaderModule(pDevice, fragmentFilepath);

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
        vkDestroyShaderModule(m_pDevice->m_deviceHandle, m_shaders[i], nullptr);
    }
}

VulkanPipelineLayout::VulkanPipelineLayout()
    :m_pDevice(nullptr)
    ,m_layoutHandle(VK_NULL_HANDLE)
{
}

void VulkanPipelineLayout::Init(const VulkanDevice* pDevice, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
    m_pDevice = pDevice;

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = (U32)descriptorSetLayouts.size();
    layoutInfo.pSetLayouts = descriptorSetLayouts.data();
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = nullptr;

    SM_VULKAN_ASSERT(vkCreatePipelineLayout(m_pDevice->m_deviceHandle, &layoutInfo, nullptr, &m_layoutHandle));
}

void VulkanPipelineLayout::Destroy()
{
    vkDestroyPipelineLayout(m_pDevice->m_deviceHandle, m_layoutHandle, nullptr);
}

VulkanMeshPipelineInputInfo::VulkanMeshPipelineInputInfo()
    :m_inputAssemblyInfo({})
    ,m_vertexInputInfo({})
{
}

void VulkanMeshPipelineInputInfo::Init(const Mesh* pMesh, bool primitiveRestartEnabled)
{
    SM_ASSERT(pMesh != nullptr);

	m_inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    switch (pMesh->m_topology)
    {
		case PrimitiveTopology::kPointList: m_inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case PrimitiveTopology::kLineList: m_inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case PrimitiveTopology::kTriangleList: m_inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
	m_inputAssemblyInfo.primitiveRestartEnable = primitiveRestartEnabled;

    m_vertexInputBindingDescs = VulkanFormats::GetVertexInputBindingDescs(VertexType::kPCT);
    m_vertexInputAttrDescs = VulkanFormats::GetVertexInputAttrDescs(VertexType::kPCT);

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
    ,m_bDepthStencilState(false)
    ,m_bDidSampleColorBlendState(false)
{
}

void VulkanPipelineState::PreInitAddColorBlendAttachment();

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
	m_scissor.offset = { scissorOffsetX, scissorOffsetY };
	m_scissor.extent = { scissorExtentX, scissorExtentY };

	m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_viewportState.pViewports = &m_viewport;
	m_viewportState.viewportCount = 1;
	m_viewportState.pScissors = &m_scissor;
	m_viewportState.scissorCount = 1;
}

void VulkanPipelineState::InitMultisampleState();
void VulkanPipelineState::InitDepthStencilState();
void VulkanPipelineState::InitColorBlendState();

bool VulkanPipelineState::IsFullyInitialized() const
{
    return m_bDidInitRasterState &&
           m_bDidInitViewportState &&
           m_bDidInitMultisampleState &&
           m_bDepthStencilState &&
           m_bDidSampleColorBlendState;
}
