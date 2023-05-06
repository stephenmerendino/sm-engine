#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Core/Types.h"

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