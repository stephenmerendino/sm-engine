#include "engine/render/vulkan/VulkanCommands.h"
#include "engine/render/vulkan/VulkanCommandPool.h"
#include "engine/render/vulkan/VulkanDevice.h"
#include "engine/render/vulkan/VulkanRenderPass.h"
#include "engine/render/vulkan/VulkanPipeline.h"
#include "engine/render/RenderableMesh.h"
#include "engine/core/Assert.h"
#include "engine/core/Common.h"

void VulkanCommands::CopyBuffer(VulkanCommandPool* pCommandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer copyCommandBuffer = pCommandPool->BeginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(copyCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	pCommandPool->EndSingleTimeCommands(copyCommandBuffer);
}

void VulkanCommands::GenerateMipMaps(VulkanCommandPool* pCommandPool, VulkanDevice* pDevice, VkImage image, VkFormat format, I32 texWidth, I32 texHeight, U32 numMipLevels)
{
	// Check if image format supports linear filtered blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(pDevice->GetPhysicalDeviceHandle(), format, &formatProperties);
	ASSERT(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
	
	VkCommandBuffer commandBuffer = pCommandPool->BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	I32 mipWidth = texWidth;
	I32 mipHeight = texHeight;

	for (U32 i = 1; i < numMipLevels; ++i)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.srcSubresource.mipLevel = i - 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;
		blit.dstSubresource.mipLevel = i;

		vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = numMipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	pCommandPool->EndSingleTimeCommands(commandBuffer);
}

void VulkanCommands::TransitionImageLayout(VulkanCommandPool* pCommandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, U32 numMipLevels)
{
	Unused(format);

	VkCommandBuffer commandBuffer = pCommandPool->BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = numMipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	bool isProperTransition = false;
	VkPipelineStageFlags srcStage = 0;
	VkPipelineStageFlags dstStage = 0;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		isProperTransition = true;

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		isProperTransition = true;

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	ASSERT(isProperTransition);

	vkCmdPipelineBarrier(commandBuffer, 
						 srcStage, dstStage, 
						 0, 
						 0, nullptr, 
						 0, nullptr, 
						 1, &barrier);

	pCommandPool->EndSingleTimeCommands(commandBuffer);
}

void VulkanCommands::CopyBufferToImage(VulkanCommandPool* pCommandPool, VkBuffer buffer, VkImage image, U32 width, U32 height)
{
	VkCommandBuffer commandBuffer = pCommandPool->BeginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferImageHeight = 0;
	region.bufferRowLength = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	pCommandPool->EndSingleTimeCommands(commandBuffer);
}

void VulkanCommands::BeginCommandBuffer(VkCommandBuffer commandBuffer)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pInheritanceInfo = nullptr;
	beginInfo.flags = 0;
	VULKAN_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
}

void VulkanCommands::EndCommandBuffer(VkCommandBuffer commandBuffer)
{
	VULKAN_ASSERT(vkEndCommandBuffer(commandBuffer));
}

void VulkanCommands::BeginRenderPass(VkCommandBuffer commandBuffer, VulkanRenderPass* pRenderPass, VkFramebuffer framebuffer, VkOffset2D offset, VkExtent2D extent, const std::vector<VkClearValue>& clearColors)
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = pRenderPass->GetHandle();
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.offset = offset;// { 0, 0 };
	renderPassInfo.renderArea.extent = extent; // m_pSwapchain->GetExtent();
	renderPassInfo.clearValueCount = static_cast<U32>(clearColors.size());
	renderPassInfo.pClearValues = clearColors.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommands::EndRenderPass(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}

void VulkanCommands::DrawRenderableMesh(VkCommandBuffer commandBuffer, RenderableMesh* pMesh, VulkanPipeline* pPipeline, const std::vector<VkDescriptorSet> descriptorSets)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->GetPipelineHandle());

	VkBuffer vertexBuffers[] = { pMesh->GetVertexBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffer, pMesh->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->GetPipelineLayoutHandle(), 0, (U32)descriptorSets.size(), descriptorSets.data(), 0, nullptr);

	vkCmdDrawIndexed(commandBuffer, pMesh->GetNumIndices(), 1, 0, 0, 0);
}
