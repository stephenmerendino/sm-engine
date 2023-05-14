#include "engine/render/Vertex.h"
#include "engine/core/macros.h"

VkVertexInputBindingDescription get_binding_desc(const vertex_pct_t& vtx)
{
	VkVertexInputBindingDescription vertexInputBindingDesc = {};
	vertexInputBindingDesc.binding = 0;
	vertexInputBindingDesc.stride = sizeof(vtx);
	vertexInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return vertexInputBindingDesc;
}

std::vector<VkVertexInputAttributeDescription> get_attr_descs(const vertex_pct_t& vtx)
{
    UNUSED(vtx);

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
