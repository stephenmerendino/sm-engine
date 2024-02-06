#include "Engine/Render/Vulkan/VulkanTexture.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Debug.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Render/Vulkan/VulkanCommands.h"
#include "Engine/Render/Vulkan/VulkanBuffer.h"

#pragma warning(push)
#pragma warning(disable:4244)
#define STB_IMAGE_IMPLEMENTATION
#include "Engine/ThirdParty/stb/stb_image.h"
#pragma warning(pop)

#include <cmath>

static VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, U32 numMips)
{
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = numMips;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView image_view;
	SM_VULKAN_ASSERT(vkCreateImageView(VulkanDevice::GetHandle(), &createInfo, nullptr, &image_view));
	return image_view;
}

static void CreateImage(U32 width, U32 height, U32 numMips, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memProps, VkImage* outImage, VkDeviceMemory* outMemory)
{
	// Create vulkan image object
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = numMips;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = format;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = numSamples;
	imageCreateInfo.flags = 0;

	SM_VULKAN_ASSERT(vkCreateImage(VulkanDevice::GetHandle(), &imageCreateInfo, nullptr, outImage));

	// Setup backing memory of image
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(VulkanDevice::GetHandle(), *outImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = VulkanDevice::Get()->FindSupportedMemoryType(memRequirements.memoryTypeBits, memProps);

	SM_VULKAN_ASSERT(vkAllocateMemory(VulkanDevice::GetHandle(), &allocInfo, nullptr, outMemory));

	vkBindImageMemory(VulkanDevice::GetHandle(), *outImage, *outMemory, 0);
}

VulkanTexture::VulkanTexture()
	:m_image(VK_NULL_HANDLE)
	,m_imageView(VK_NULL_HANDLE)
	,m_deviceMemory(VK_NULL_HANDLE)
	,m_numMips(0)
{
}

void VulkanTexture::InitFromFile(const VulkanCommandPool& commandPool, const char* filepath)
{
	int texWidth;
	int texHeight;
	int texChannels;

	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

	// load image pixels
	stbi_uc* pixels = stbi_load(filepath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	// calc num mips
	m_numMips = (U32)(std::floor(std::log2(Max(texWidth, texHeight))) + 1);

	// Copy image data to staging buffer
	VkDeviceSize imageSize = texWidth * texHeight * 4; // times 4 because of STBI_rgb_alpha
	VulkanBuffer stagingBuffer;
	stagingBuffer.Init(VulkanBuffer::Type::kStagingBuffer, imageSize);
	stagingBuffer.Update(commandPool, pixels, 0);

	// Make sure to free the pixels data
	stbi_image_free(pixels);

	// Create the vulkan image and its memory
	CreateImage(texWidth, texHeight, m_numMips, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_image, &m_deviceMemory);

	// Transition the image to transfer destination layout
	VkCommandBuffer commandBuffer = commandPool.BeginSingleTime();
		VulkanCommands::TransitionImageLayout(commandBuffer, m_image, m_numMips, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
											  VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT);

		// Copy pixel data from staging buffer to actual vulkan image
		VulkanCommands::CopyBufferToImage(commandBuffer, stagingBuffer.m_bufferHandle, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texWidth, texHeight);

		// Transition image to shader read layout so it can be used in fragment shader
		//command_generate_mip_maps(context, texture.handle, format, tex_width, tex_height, texture.num_mips);
		VulkanCommands::GenerateMipMaps(commandBuffer, m_image, format, texWidth, texHeight, m_numMips);
	commandPool.EndAndSubmitSingleTime(commandBuffer);

	// Set user friendly debug name
	if (IsDebug())
	{
		VkDebugUtilsObjectNameInfoEXT debugNameInfo = {};
		debugNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		debugNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
		debugNameInfo.objectHandle = (U64)m_image;
		debugNameInfo.pObjectName = filepath;
		debugNameInfo.pNext = nullptr;

		vkSetDebugUtilsObjectNameEXT(VulkanDevice::GetHandle(), &debugNameInfo);
	}

	stagingBuffer.Destroy();

	m_imageView = CreateImageView(m_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_numMips);
}

void VulkanTexture::InitColorTarget(VkFormat format, U32 width, U32 height, VkImageUsageFlags usage, VkSampleCountFlagBits numSamples)
{
	m_numMips = 1;
	CreateImage(width, height,
				m_numMips,
				numSamples,
				format,
				VK_IMAGE_TILING_OPTIMAL,
				usage,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&m_image,
				&m_deviceMemory);
	m_imageView = CreateImageView(m_image, format, VK_IMAGE_ASPECT_COLOR_BIT, m_numMips);
}

void VulkanTexture::InitDepthTarget(VkFormat format, U32 width, U32 height, VkImageUsageFlags usage, VkSampleCountFlagBits numSamples)
{
	m_numMips = 1;
	CreateImage(width, height,
				m_numMips,
				numSamples,
				format,
				VK_IMAGE_TILING_OPTIMAL,
				usage,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&m_image,
				&m_deviceMemory);
	m_imageView = CreateImageView(m_image, format, VK_IMAGE_ASPECT_DEPTH_BIT, m_numMips);
}

void VulkanTexture::Destroy()
{
	vkDestroyImageView(VulkanDevice::GetHandle(), m_imageView, nullptr);
	vkDestroyImage(VulkanDevice::GetHandle(), m_image, nullptr);
	vkFreeMemory(VulkanDevice::GetHandle(), m_deviceMemory, nullptr);
}
