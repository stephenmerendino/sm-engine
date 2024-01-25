#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Core/Types.h"

namespace VulkanCommands
{
	void TransitionImageLayout(VkCommandBuffer commandBuffer, 
							   VkImage image, 
							   U32 num_mips, 
							   VkImageLayout oldLayout,
							   VkImageLayout newLayout,
							   VkPipelineStageFlags srcStage,
							   VkPipelineStageFlags dstStage,
							   VkAccessFlags srcAccess,
							   VkAccessFlags dstAccess);
}
