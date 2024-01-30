#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Render/Vertex.h"
#include <vector>

namespace VulkanFormats
{
	void Init(const VulkanDevice& device);

	VkFormat GetMainColorFormat();
	VkFormat GetMainDepthFormat();
	bool FormatHasStencilComponent(VkFormat format);

	std::vector<VkVertexInputBindingDescription> GetVertexInputBindingDescs(VertexType vertexType);
	std::vector<VkVertexInputAttributeDescription> GetVertexInputAttrDescs(VertexType vertexType);
}
