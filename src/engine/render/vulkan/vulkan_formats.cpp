#include "engine/render/vulkan/vulkan_formats.h"

bool format_has_stencil_component(VkFormat format)
{
    return (format == VK_FORMAT_D32_SFLOAT_S8_UINT) ||
           (format == VK_FORMAT_D24_UNORM_S8_UINT);
}

VkFormat format_find_supported(device_t& device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(device.phys_device_handle, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	return VK_FORMAT_UNDEFINED;
}

VkFormat format_find_depth(device_t& device)
{
	VkFormat depth_format = format_find_supported(device, 
                                                  {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
												  VK_IMAGE_TILING_OPTIMAL, 
												  VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	ASSERT(depth_format != VK_FORMAT_UNDEFINED);
	return depth_format;
} 

u32 memory_find_type(context_t& context, u32 type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(context.device.phys_device_handle, &mem_properties);

    u32 found_mem_type = UINT_MAX;
    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++)
    {
        if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            found_mem_type = i;
            break;
        }
    }

    ASSERT(found_mem_type != UINT_MAX);
    return found_mem_type;
}
