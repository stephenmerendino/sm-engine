#include "Engine/Render/Vulkan/VulkanPipeline.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Core/Types.h"
#include "Engine/Core/File.h"

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
