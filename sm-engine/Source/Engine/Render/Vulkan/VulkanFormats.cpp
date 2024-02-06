#include "Engine/Render/Vulkan/VulkanFormats.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"

static VkFormat s_mainDepthFormat = VK_FORMAT_UNDEFINED;

namespace VulkanFormats
{
	void Init()
	{
		s_mainDepthFormat = VulkanDevice::Get()->FindSupportedDepthFormat();
	}

	VkFormat GetMainColorFormat()
	{
		return VK_FORMAT_R8G8B8A8_UNORM;
	}

	VkFormat GetMainDepthFormat()
	{
		return s_mainDepthFormat;
	}

	bool FormatHasStencilComponent(VkFormat format)
	{
		return (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || 
			   (format == VK_FORMAT_D24_UNORM_S8_UINT);
	}

	std::vector<VkVertexInputBindingDescription> GetVertexInputBindingDescs(VertexType vertexType)
	{
		std::vector<VkVertexInputBindingDescription> vertexInputBindingDescs;

		if (vertexType == VertexType::kPCT)
		{
			vertexInputBindingDescs.resize(1);
			vertexInputBindingDescs[0].binding = 0;
			vertexInputBindingDescs[0].stride = sizeof(VertexPCT);
			vertexInputBindingDescs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}

		return vertexInputBindingDescs;
	}

	std::vector<VkVertexInputAttributeDescription> GetVertexInputAttrDescs(VertexType vertexType)
	{
		std::vector<VkVertexInputAttributeDescription> attr_descs;

		if (vertexType == VertexType::kPCT)
		{
			attr_descs.resize(3);

			//pos
			VkVertexInputAttributeDescription pos;
			pos.binding = 0;
			pos.location = 0;
			pos.offset = offsetof(VertexPCT, m_pos);
			pos.format = VK_FORMAT_R32G32B32_SFLOAT;

			// color
			VkVertexInputAttributeDescription color;
			color.binding = 0;
			color.location = 1;
			color.offset = offsetof(VertexPCT, m_color);
			color.format = VK_FORMAT_R32G32B32A32_SFLOAT;

			// tex coord
			VkVertexInputAttributeDescription uv;
			uv.binding = 0;
			uv.location = 2;
			uv.offset = offsetof(VertexPCT, m_uv);
			uv.format = VK_FORMAT_R32G32_SFLOAT;

			attr_descs[0] = pos;
			attr_descs[1] = color;
			attr_descs[2] = uv;
		}

		return attr_descs;
	}
}
