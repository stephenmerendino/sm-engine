#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"

class VulkanSampler
{
public:
	VulkanSampler();

	void Init(const VulkanDevice& device, U32 maxMipLevel);
	void Destroy();

	const VulkanDevice* m_pDevice;
	VkSampler m_samplerHandle; 
    U32 m_maxMipLevel = 1;
};
