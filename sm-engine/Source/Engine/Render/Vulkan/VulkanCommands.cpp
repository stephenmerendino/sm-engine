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

	void CopyBuffer(VkCommandBuffer commandBuffer, VulkanBuffer& src, VulkanBuffer& dst, VkDeviceSize copySize)
	{
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = copySize;
		vkCmdCopyBuffer(commandBuffer, src.m_bufferHandle, dst.m_bufferHandle, 1, &copyRegion);
	}

	void GenerateMipMaps(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, U32 width, U32 height, U32 numMips)
	{
		// Check if image format supports linear filtered blitting
		VkFormatProperties formatProps = VulkanDevice::Get()->QueryFormatProperties(format);;
		SM_ASSERT(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		I32 mipWidth = (I32)width;
		I32 mipHeight = (I32)height;

		for (U32 i = 1; i < numMips; ++i)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			BlitColorImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mipWidth, mipHeight, 1, i - 1,
						   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1, i);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = numMips - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, VkImageLayout imageLayout, U32 width, U32 height)
	{
		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferImageHeight = 0;
		region.bufferRowLength = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		vkCmdCopyBufferToImage(commandBuffer, buffer, image, imageLayout, 1, &region);
	}

	void CopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcLayout, VkImage dstImage, VkImageLayout dstLayout, U32 width, U32 height, U32 depth)
	{
		VkImageCopy copy = {};
		copy.extent = { width, height, depth };
		copy.srcSubresource.mipLevel = 0;
		copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.srcSubresource.layerCount = 1;
		copy.srcSubresource.baseArrayLayer = 0;
		copy.dstSubresource.mipLevel = 0;
		copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.dstSubresource.layerCount = 1;
		copy.dstSubresource.baseArrayLayer = 0;
		vkCmdCopyImage(commandBuffer, srcImage, srcLayout, dstImage, dstLayout, 1, &copy);
	}

	static void InternalBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, U32 srcWidth, U32 srcHeight, U32 srcDepth, U32 srcMipLevel,
								  VkImage dstImage, VkImageLayout dstImageLayout, U32 dstWidth, U32 dstHeight, U32 dstDepth, U32 dstMipLevel,
								  VkFilter filter, VkImageAspectFlagBits imageAspect)
	{
		VkImageBlit blitInfo = {};

		VkImageSubresourceLayers srcSubresource = {};
		srcSubresource.mipLevel = srcMipLevel;
		srcSubresource.aspectMask = imageAspect;
		srcSubresource.layerCount = 1;
		srcSubresource.baseArrayLayer = 0;

		blitInfo.srcSubresource = srcSubresource;

		VkImageSubresourceLayers dstSubresource = {};
		dstSubresource.mipLevel = dstMipLevel;
		dstSubresource.aspectMask = imageAspect;
		dstSubresource.layerCount = 1;
		dstSubresource.baseArrayLayer = 0;

		blitInfo.dstSubresource = dstSubresource;

		VkOffset3D zeroOffset = {};
		zeroOffset.x = 0;
		zeroOffset.y = 0;
		zeroOffset.z = 0;

		VkOffset3D srcFullExtent = {};
		srcFullExtent.x = srcWidth;
		srcFullExtent.y = srcHeight;
		srcFullExtent.z = srcDepth;

		VkOffset3D dstFullExtent = {};
		dstFullExtent.x = dstWidth;
		dstFullExtent.y = dstHeight;
		dstFullExtent.z = dstDepth;

		blitInfo.srcOffsets[0] = zeroOffset;
		blitInfo.srcOffsets[1] = srcFullExtent;
		blitInfo.dstOffsets[0] = zeroOffset;
		blitInfo.dstOffsets[1] = dstFullExtent;

		vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, 1, &blitInfo, filter);
	}

	void BlitColorImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, U32 srcWidth, U32 srcHeight, U32 srcDepth, U32 srcMipLevel,
						VkImage dstImage, VkImageLayout dstImageLayout, U32 dstWidth, U32 dstHeight, U32 dstDepth, U32 dstMipLevel,
						VkFilter filter)
	{
		InternalBlitImage(commandBuffer, srcImage, srcImageLayout, srcWidth, srcHeight, srcDepth, srcMipLevel,
						  dstImage, dstImageLayout, dstWidth, dstHeight, dstDepth, dstMipLevel,
						  filter, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void BlitDepthImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, U32 srcWidth, U32 srcHeight, U32 srcDepth, U32 srcMipLevel,
						VkImage dstImage, VkImageLayout dstImageLayout, U32 dstWidth, U32 dstHeight, U32 dstDepth, U32 dstMipLevel,
						VkFilter filter)
	{
		InternalBlitImage(commandBuffer, srcImage, srcImageLayout, srcWidth, srcHeight, srcDepth, srcMipLevel,
						  dstImage, dstImageLayout, dstWidth, dstHeight, dstDepth, dstMipLevel,
						  filter, VK_IMAGE_ASPECT_DEPTH_BIT);
	}


	void BeginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, VkOffset2D offset, VkExtent2D extent, const std::vector<VkClearValue>& clearColors)
	{
		VkRenderPassBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		begin_info.renderPass = renderPass;
		begin_info.framebuffer = framebuffer;
		begin_info.renderArea.offset = offset;
		begin_info.renderArea.extent = extent;
		begin_info.clearValueCount = static_cast<U32>(clearColors.size());
		begin_info.pClearValues = clearColors.data();

		vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
	}

	void BeginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, VkOffset2D offset, VkExtent2D extent)
	{
		VkRenderPassBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		begin_info.renderPass = renderPass;
		begin_info.framebuffer = framebuffer;
		begin_info.renderArea.offset = offset;
		begin_info.renderArea.extent = extent;
		begin_info.clearValueCount = 0;
		begin_info.pClearValues = nullptr;

		vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
	}

	void EndRenderPass(VkCommandBuffer commandBuffer)
	{
		vkCmdEndRenderPass(commandBuffer);
	}
}