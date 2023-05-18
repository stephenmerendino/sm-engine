#include "engine/render/vulkan/vulkan_temp.h"

VkImageView image_view_create(device_t& device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, u32 num_mips)
{
    VkImageViewCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = format;
    create_info.subresourceRange.aspectMask = aspect_flags;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = num_mips;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    VULKAN_ASSERT(vkCreateImageView(device.m_device_handle, &create_info, nullptr, &image_view));
    return image_view;
}

VkImageView image_view_create(context_t& context, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, u32 num_mips)
{
    return image_view_create(context.m_device, image, format, aspect_flags, num_mips);
}

