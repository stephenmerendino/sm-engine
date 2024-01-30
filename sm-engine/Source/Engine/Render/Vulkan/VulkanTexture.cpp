#include "Engine/Render/Vulkan/VulkanTexture.h"
#include "Engine/Core/Assert.h"

static VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, U32 numMips)
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
	SM_VULKAN_ASSERT(vkCreateImageView(device, &createInfo, nullptr, &image_view));
	return image_view;
}

static VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, U32 numMips)
{
	return CreateImageView(device, image, format, aspectFlags, numMips);
}

static void CreateImage(const VulkanDevice& device, U32 width, U32 height, U32 numMips, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memProps, VkImage& outImage, VkDeviceMemory& outMemory)
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

	SM_VULKAN_ASSERT(vkCreateImage(device.m_deviceHandle, &imageCreateInfo, nullptr, &outImage));

	// Setup backing memory of image
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device.m_deviceHandle, outImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device.FindSupportedMemoryType(memRequirements.memoryTypeBits, memProps);

	SM_VULKAN_ASSERT(vkAllocateMemory(device.m_deviceHandle, &allocInfo, nullptr, &outMemory));

	vkBindImageMemory(device.m_deviceHandle, outImage, outMemory, 0);
}

VulkanTexture::VulkanTexture()
	:m_device(VK_NULL_HANDLE)
	,m_image(VK_NULL_HANDLE)
	,m_imageView(VK_NULL_HANDLE)
	,m_deviceMemory(VK_NULL_HANDLE)
	,m_numMips(0)
{
}

void VulkanTexture::InitFromFile(const VulkanDevice& device, const char* filepath)
{
	//texture_t texture;

	//int tex_width;
	//int tex_height;
	//int tex_channels;

	//VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

	//// load image pixels
	//stbi_uc* pixels = stbi_load(filepath, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

	//// calc num mips
	//texture.num_mips = (u32)(std::floor(std::log2(std::max(tex_width, tex_height))) + 1);

	//// Copy image data to staging buffer
	//VkDeviceSize image_size = tex_width * tex_height * 4; // times 4 because of STBI_rgb_alpha
	//buffer_t staging_buffer = buffer_create(context, BufferType::kStagingBuffer, image_size);

	//// Do the actual memcpy on cpu side
	//buffer_update_data(context, staging_buffer, pixels);

	//// Make sure to free the pixels data
	//stbi_image_free(pixels);

	//// Create the vulkan image and its memory
	//image_create(context, tex_width, tex_height, texture.num_mips, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.handle, texture.memory);

	//// Transition the image to transfer destination layout
	//VkCommandBuffer transition_command = command_begin_single_time(context);
	//command_transition_image_layout(transition_command, texture.handle, texture.num_mips, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//	VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
	//	VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT);
	//command_end_single_time(context, transition_command);

	//// Copy pixel data from staging buffer to actual vulkan image
	//command_copy_buffer_to_image(context, staging_buffer.handle, texture.handle, tex_width, tex_height);

	//// Transition image to shader read layout so it can be used in fragment shader
	//command_generate_mip_maps(context, texture.handle, format, tex_width, tex_height, texture.num_mips);

	//// Set user friendly debug name
	//if (is_debug())
	//{
	//	VkDebugUtilsObjectNameInfoEXT debug_name_info = {};
	//	debug_name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	//	debug_name_info.objectType = VK_OBJECT_TYPE_IMAGE;
	//	debug_name_info.objectHandle = (u64)texture.handle;
	//	debug_name_info.pObjectName = filepath;
	//	debug_name_info.pNext = nullptr;

	//	vkSetDebugUtilsObjectNameEXT(context.device.device_handle, &debug_name_info);
	//}

	//buffer_destroy(context, staging_buffer);

	//texture.image_view = image_view_create(context, texture.handle, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, texture.num_mips);

	//return texture;
}

void VulkanTexture::InitColorTarget(const VulkanDevice& device, VkFormat format, U32 width, U32 height, VkImageUsageFlags usage, VkSampleCountFlagBits numSamples)
{
	m_numMips = 1;
	CreateImage(device,
				width, height,
				m_numMips,
				numSamples,
				format,
				VK_IMAGE_TILING_OPTIMAL,
				usage,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				m_image,
				m_deviceMemory);
	m_imageView = CreateImageView(device.m_deviceHandle, m_image, format, VK_IMAGE_ASPECT_COLOR_BIT, m_numMips);
}

void VulkanTexture::InitDepthTarget(const VulkanDevice& device, VkFormat format, U32 width, U32 height, VkImageUsageFlags usage, VkSampleCountFlagBits numSamples)
{
	m_numMips = 1;
	CreateImage(device,
				width, height,
				m_numMips,
				numSamples,
				format,
				VK_IMAGE_TILING_OPTIMAL,
				usage,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				m_image,
				m_deviceMemory);
	m_imageView = CreateImageView(device.m_deviceHandle, m_image, format, VK_IMAGE_ASPECT_DEPTH_BIT, m_numMips);
}

void VulkanTexture::Destroy()
{
	vkDestroyImageView(m_device, m_imageView, nullptr);
	vkDestroyImage(m_device, m_image, nullptr);
	vkFreeMemory(m_device, m_deviceMemory, nullptr);
}
