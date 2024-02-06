#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"

class VulkanSampler
{
public:
	VulkanSampler();

	void Init(U32 maxMipLevel);
	void Destroy();

	VkSampler m_samplerHandle; 
    U32 m_maxMipLevel;
};
