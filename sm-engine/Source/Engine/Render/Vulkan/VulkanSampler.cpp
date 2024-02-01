#include "Engine/Render/Vulkan/VulkanSampler.h"

VulkanSampler::VulkanSampler()
	:m_pDevice(nullptr)
    ,m_samplerHandle(VK_NULL_HANDLE)
	,m_maxMipLevel(1)
{
}

void VulkanSampler::Init(const VulkanDevice& device, U32 maxMipLevel)
{
    m_pDevice = &device;
	m_maxMipLevel = maxMipLevel;

    VkSamplerCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = device.m_physicalDeviceProps.limits.maxSamplerAnisotropy;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = (F32)m_maxMipLevel;

    SM_VULKAN_ASSERT(vkCreateSampler(device.m_deviceHandle, &createInfo, nullptr, &m_samplerHandle));
}

void VulkanSampler::Destroy()
{
    vkDestroySampler(m_pDevice->m_deviceHandle, m_samplerHandle, nullptr);
}
