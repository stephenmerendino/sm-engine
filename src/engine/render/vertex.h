#pragma once

#include "engine/math/vec2.h"
#include "engine/math/vec3.h"
#include "engine/math/vec4.h"
#include "engine/render/vulkan/vulkan_functions.h"

#include <vector>

struct vertex_pct_t
{
   vec3 m_pos;
   vec4 m_color;
   vec2 m_uv;
};

std::vector<VkVertexInputBindingDescription> get_vertex_input_binding_descs(const vertex_pct_t& v);
std::vector<VkVertexInputAttributeDescription> get_vertex_input_attr_descs(const vertex_pct_t& v);
