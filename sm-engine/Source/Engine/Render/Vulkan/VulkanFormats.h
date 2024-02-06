#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vertex.h"
#include <vector>

namespace VulkanFormats
{
	void Init();

	VkFormat GetMainColorFormat();
	VkFormat GetMainDepthFormat();
	bool FormatHasStencilComponent(VkFormat format);

	std::vector<VkVertexInputBindingDescription> GetVertexInputBindingDescs(VertexType vertexType);
	std::vector<VkVertexInputAttributeDescription> GetVertexInputAttrDescs(VertexType vertexType);
}
