#include "Engine/Render/Vulkan/VulkanCommands.h"

namespace VulkanCommands
{
	void Begin(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pInheritanceInfo = nullptr;
		beginInfo.flags = usageFlags;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
	}

	void End(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);
	}

	void TransitionImageLayout(VkCommandBuffer commandBuffer, 
							   VkImage image, 
							   U32 num_mips, 
							   VkImageLayout oldLayout,
							   VkImageLayout newLayout,
							   VkPipelineStageFlags srcStage,
							   VkPipelineStageFlags dstStage,
							   VkAccessFlags srcAccess,
							   VkAccessFlags dstAccess)
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcAccessMask = srcAccess;
		barrier.dstAccessMask = dstAccess;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = num_mips;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(commandBuffer,
							 srcStage, dstStage,
							 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);
	}

}