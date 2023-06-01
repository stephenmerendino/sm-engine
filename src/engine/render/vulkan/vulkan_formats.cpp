#include "engine/render/vulkan/vulkan_formats.h"
#include "engine/core/macros.h"

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

std::vector<VkVertexInputBindingDescription> get_vertex_input_binding_descs(const vertex_pct_t& v)
{
    UNUSED(v);

    std::vector<VkVertexInputBindingDescription> vertex_input_binding_descs;
    vertex_input_binding_descs.resize(1);
    vertex_input_binding_descs[0].binding = 0;
    vertex_input_binding_descs[0].stride = sizeof(v);
    vertex_input_binding_descs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return vertex_input_binding_descs;
}

std::vector<VkVertexInputAttributeDescription> get_vertex_input_attr_descs(const vertex_pct_t& v)
{
    UNUSED(v);

	std::vector<VkVertexInputAttributeDescription> attr_descs;
	attr_descs.resize(3);

	//pos
	VkVertexInputAttributeDescription pos;
	pos.binding = 0;
	pos.location = 0;
	pos.offset = offsetof(vertex_pct_t, m_pos);
	pos.format = VK_FORMAT_R32G32B32_SFLOAT;

	// color
	VkVertexInputAttributeDescription color;
	color.binding = 0;
	color.location = 1;
	color.offset = offsetof(vertex_pct_t, m_color);
	color.format = VK_FORMAT_R32G32B32A32_SFLOAT;

	// tex coord
	VkVertexInputAttributeDescription uv;
	uv.binding = 0;
	uv.location = 2;
	uv.offset = offsetof(vertex_pct_t, m_uv);
	uv.format = VK_FORMAT_R32G32_SFLOAT;

	attr_descs[0] = pos;
	attr_descs[1] = color;
	attr_descs[2] = uv;

	return attr_descs;
}

std::vector<VkVertexInputBindingDescription> mesh_get_vertex_input_binding_descs(mesh_t* mesh)
{
    ASSERT(nullptr != mesh);
	return get_vertex_input_binding_descs(mesh->vertices[0]);
}

std::vector<VkVertexInputAttributeDescription> mesh_get_vertex_input_attr_descs(mesh_t* mesh)
{
    ASSERT(nullptr != mesh);
	return get_vertex_input_attr_descs(mesh->vertices[0]);
}
