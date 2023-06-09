#pragma once

#include "engine/render/vulkan/vulkan_types.h"

VkFormat format_find_depth(device_t& device);
VkFormat format_find_supported(device_t& device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
bool format_has_stencil_component(VkFormat format);

u32 memory_find_type(context_t& context, u32 type_filter, VkMemoryPropertyFlags properties);

std::vector<VkVertexInputBindingDescription> mesh_get_vertex_input_binding_descs(mesh_t* mesh);
std::vector<VkVertexInputAttributeDescription> mesh_get_vertex_input_attr_descs(mesh_t* mesh);
