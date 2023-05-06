#pragma once

#include "engine/render/vulkan/VulkanInclude.h"
#include "engine/core/Types.h"

class VulkanDevice;

class VulkanSampler
{
public:
	VulkanSampler(VulkanDevice* pDevice);
	~VulkanSampler();

	bool Setup(U32 maxMipLevel);
	void Teardown();

	VkSampler GetHandle();

private:
	VkSampler m_vkSampler;
	VulkanDevice* m_pDevice;
	U32 m_maxMipLevel;
};
