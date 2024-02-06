#include "Engine/Render/Vulkan/VulkanSampler.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"

VulkanSampler::VulkanSampler()
    :m_samplerHandle(VK_NULL_HANDLE)
	,m_maxMipLevel(1)
{
}

void VulkanSampler::Init(U32 maxMipLevel)
{
	m_maxMipLevel = maxMipLevel;

    VkSamplerCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = VulkanDevice::Get()->m_physicalDeviceProps.limits.maxSamplerAnisotropy;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = (F32)m_maxMipLevel;

    SM_VULKAN_ASSERT(vkCreateSampler(VulkanDevice::GetHandle(), &createInfo, nullptr, &m_samplerHandle));
}

void VulkanSampler::Destroy()
{
    vkDestroySampler(VulkanDevice::GetHandle(), m_samplerHandle, nullptr);
}
