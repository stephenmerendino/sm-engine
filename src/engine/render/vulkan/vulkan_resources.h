#pragma once

#include "engine/render/vulkan/vulkan_types.h"

VkImageView image_view_create(device_t& device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, u32 num_mips);
VkImageView image_view_create(context_t& context, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, u32 num_mips);
VkFormat format_find_depth(device_t& device);
VkFormat format_find_supported(device_t& device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
bool format_has_stencil_component(VkFormat format);
