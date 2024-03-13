#pragma once

#include "Engine/Render/Vulkan/VulkanBuffer.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Core/Color.h"
#include "Engine/Core/Types.h"

namespace VulkanCommands
{
	void Begin(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags);
	void End(VkCommandBuffer commandBuffer);

	void TransitionImageLayout(VkCommandBuffer commandBuffer, 
							   VkImage image, 
							   U32 numMips, 
							   VkImageLayout oldLayout,
							   VkImageLayout newLayout,
							   VkPipelineStageFlags srcStage,
							   VkPipelineStageFlags dstStage,
							   VkAccessFlags srcAccess,
							   VkAccessFlags dstAccess);

	void CopyBuffer(VkCommandBuffer commandBuffer, VulkanBuffer& src, VulkanBuffer& dst, VkDeviceSize copySize);

	void GenerateMipMaps(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, U32 width, U32 height, U32 numMips);

	void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, VkImageLayout imageLayout, U32 width, U32 height);

	void CopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcLayout, VkImage dstImage, VkImageLayout dstLayout, U32 width, U32 height, U32 depth = 1);

	void BlitColorImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, U32 srcWidth, U32 srcHeight, U32 srcDepth, U32 srcMipLevel,
						VkImage dstImage, VkImageLayout dstImageLayout, U32 dstWidth, U32 dstHeight, U32 dstDepth, U32 dstMipLevel,
						VkFilter filter = VK_FILTER_LINEAR);

	void BlitDepthImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, U32 srcWidth, U32 srcHeight, U32 srcDepth, U32 srcMipLevel,
						VkImage dstImage, VkImageLayout dstImageLayout, U32 dstWidth, U32 dstHeight, U32 dstDepth, U32 dstMipLevel,
						VkFilter filter = VK_FILTER_LINEAR);

	void BeginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, VkOffset2D offset, VkExtent2D extent, const std::vector<VkClearValue>& clearColors);

	void BeginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, VkOffset2D offset, VkExtent2D extent);

	void EndRenderPass(VkCommandBuffer commandBuffer);

	void BeginDebugLabel(VkCommandBuffer commandBuffer, const char* labelName, const ColorF32& labelColor);
	void EndDebugLabel(VkCommandBuffer commandBuffer);
}
