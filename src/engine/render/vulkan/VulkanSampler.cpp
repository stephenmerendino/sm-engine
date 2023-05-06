#include "Engine/Render/Vulkan/VulkanSampler.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"

#include "Engine/Core/Assert.h"

VulkanSampler::VulkanSampler(VulkanDevice* pDevice)
	:m_pDevice(pDevice)
	,m_vkSampler(VK_NULL_HANDLE)
	,m_maxMipLevel(1)
{

}

VulkanSampler::~VulkanSampler()
{

}

bool VulkanSampler::Setup(U32 maxMipLevel)
{
	m_maxMipLevel = maxMipLevel;

	// Probably should have a wrapper class around the physical device and store its properties so we can easily retrieve when needed without having to make a vulkan call everytime
	VkPhysicalDeviceProperties properties = m_pDevice->GetPhysicalDeviceProperties();

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<F32>(maxMipLevel);

	VULKAN_ASSERT(vkCreateSampler(m_pDevice->GetHandle(), &samplerInfo, nullptr, &m_vkSampler));
	return true;
}

void VulkanSampler::Teardown()
{
	vkDestroySampler(m_pDevice->GetHandle(), m_vkSampler, nullptr);
}

VkSampler VulkanSampler::GetHandle()
{
	return m_vkSampler;
}