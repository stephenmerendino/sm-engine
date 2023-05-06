#include "engine/render/vulkan/VulkanTexture.h"
#include "engine/render/vulkan/VulkanDevice.h"
#include "engine/render/vulkan/VulkanCommandPool.h"
#include "engine/render/vulkan/VulkanCommands.h"

#include "engine/core/Debug.h"
#define STB_IMAGE_IMPLEMENTATION
#include "engine/thirdparty/stb/stb_image.h"

#include <cmath>

VulkanTexture::VulkanTexture(VulkanDevice* pDevice)
	:m_pDevice(pDevice)
	,m_vkImage(VK_NULL_HANDLE)
	,m_vkImageMemory(VK_NULL_HANDLE)
	,m_vkImageView(VK_NULL_HANDLE)
	,m_numMipLevels(0)
{
}

VulkanTexture::~VulkanTexture()
{
}

bool VulkanTexture::SetupFromTextureFile(VulkanCommandPool* pGraphicsCommandPool, const char* textureFilePath)
{
	int texWidth;
	int texHeight;
	int texChannels;

	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

	// Load image pixels
	stbi_uc* pixels = stbi_load(textureFilePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	// Copy image data to staging buffer
	VkDeviceSize imageSize = texWidth * texHeight * 4; // times 4 because of STBI_rgb_alpha
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	m_numMipLevels = static_cast<U32>(std::floor(std::log2(std::max(texWidth, texHeight))) + 1);

	m_pDevice->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	// Do the actual memcpy on cpu side
	void* data;
	vkMapMemory(m_pDevice->GetHandle(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(m_pDevice->GetHandle(), stagingBufferMemory);

	// Make sure to free the pixels data
	stbi_image_free(pixels);

	// Create the vulkan image and its memory
	m_pDevice->CreateImage(texWidth, texHeight, m_numMipLevels, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vkImage, m_vkImageMemory);

	// Transition the image to transfer destination layout
	VulkanCommands::TransitionImageLayout(pGraphicsCommandPool, m_vkImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_numMipLevels);

	// Copy pixel data from staging buffer to actual vulkan image
	VulkanCommands::CopyBufferToImage(pGraphicsCommandPool, stagingBuffer, m_vkImage, static_cast<U32>(texWidth), static_cast<U32>(texHeight));

	// Transition image to shader read layout so it can be used in fragment shader
	VulkanCommands::GenerateMipMaps(pGraphicsCommandPool, m_pDevice, m_vkImage, format, texWidth, texHeight, m_numMipLevels);

	// Set user friendly debug name
	if (IsDebug())
	{
		VkDebugUtilsObjectNameInfoEXT imageNameInfo = {};
		imageNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		imageNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
		imageNameInfo.objectHandle = (U64)(m_vkImage);
		imageNameInfo.pObjectName = textureFilePath;
		imageNameInfo.pNext = nullptr;

		vkSetDebugUtilsObjectNameEXT(m_pDevice->GetHandle(), &imageNameInfo);
	}

	vkDestroyBuffer(m_pDevice->GetHandle(), stagingBuffer, nullptr);
	vkFreeMemory(m_pDevice->GetHandle(), stagingBufferMemory, nullptr);

	m_vkImageView = m_pDevice->CreateImageView(m_vkImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_numMipLevels);

	return true;
}

bool VulkanTexture::SetupColorTarget(VkFormat format, U32 width, U32 height)
{
	m_pDevice->CreateImage(width, height, 1, m_pDevice->GetMaxMsaaSampleCount(), format, 
						   VK_IMAGE_TILING_OPTIMAL, 
						   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 
						   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
						   m_vkImage, m_vkImageMemory);

	m_vkImageView = m_pDevice->CreateImageView(m_vkImage, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	return true;
}

bool VulkanTexture::SetupDepthTarget(VkFormat format, U32 width, U32 height)
{
	m_pDevice->CreateImage(width, height, 1, m_pDevice->GetMaxMsaaSampleCount(), format, 
						  VK_IMAGE_TILING_OPTIMAL, 
						  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
						  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
						  m_vkImage, m_vkImageMemory);
	m_vkImageView = m_pDevice->CreateImageView(m_vkImage, format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	return true;
}

void VulkanTexture::Teardown()
{
	vkDestroyImageView(m_pDevice->GetHandle(), m_vkImageView, nullptr);
	vkDestroyImage(m_pDevice->GetHandle(), m_vkImage, nullptr);
	vkFreeMemory(m_pDevice->GetHandle(), m_vkImageMemory, nullptr);
}

VkImageView VulkanTexture::GetImageView()
{
	return m_vkImageView;
}

U32 VulkanTexture::GetNumMipLevels() const
{
	return m_numMipLevels;
}
