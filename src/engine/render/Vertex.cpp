#include "Engine/Render/Vertex.h"

VkVertexInputBindingDescription GetBindingDescription(const Vertex& vtx)
{
	VkVertexInputBindingDescription vertexInputBindingDesc;
	MemZero(vertexInputBindingDesc);
	vertexInputBindingDesc.binding = 0;
	vertexInputBindingDesc.stride = sizeof(vtx);
	vertexInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return vertexInputBindingDesc;
}

std::vector<VkVertexInputAttributeDescription> GetAttributeDescs(const Vertex& vtx)
{
	Unused(vtx);

	std::vector<VkVertexInputAttributeDescription> attrDescs;
	attrDescs.resize(3);

	//pos
	VkVertexInputAttributeDescription pos;
	pos.binding = 0;
	pos.location = 0;
	pos.offset = offsetof(Vertex, m_pos);
	pos.format = VK_FORMAT_R32G32B32_SFLOAT;

	// color
	VkVertexInputAttributeDescription color;
	color.binding = 0;
	color.location = 1;
	color.offset = offsetof(Vertex, m_color);
	color.format = VK_FORMAT_R32G32B32A32_SFLOAT;

	// tex coord
	VkVertexInputAttributeDescription texCoord;
	texCoord.binding = 0;
	texCoord.location = 2;
	texCoord.offset = offsetof(Vertex, m_texCoord);
	texCoord.format = VK_FORMAT_R32G32_SFLOAT;

	attrDescs[0] = pos;
	attrDescs[1] = color;
	attrDescs[2] = texCoord;

	return attrDescs;
}