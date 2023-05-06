#pragma once

#include "engine/core/Types.h"
#include "engine/render/vulkan/VulkanInclude.h"

class VulkanDevice;
class VulkanCommandPool;

class VulkanTexture
{
public:
	VulkanTexture(VulkanDevice* pDevice);
	~VulkanTexture();

	bool SetupFromTextureFile(VulkanCommandPool* pGraphicsCommandPool, const char* textureFilePath);
	bool SetupColorTarget(VkFormat format, U32 width, U32 height);
	bool SetupDepthTarget(VkFormat format, U32 width, U32 height);
	void Teardown();

	VkImageView GetImageView();
	U32 GetNumMipLevels() const;

private:
	VulkanDevice* m_pDevice;
	VkImage m_vkImage;
	VkDeviceMemory m_vkImageMemory;
	VkImageView m_vkImageView;
	U32 m_numMipLevels;
};
