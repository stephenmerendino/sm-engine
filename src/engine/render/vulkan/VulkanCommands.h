#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"

#include <vector>

class VulkanCommandPool;
class VulkanRenderPass;
class VulkanDevice;
class VulkanPipeline;
class RenderableMesh;

namespace VulkanCommands
{
	void CopyBuffer(VulkanCommandPool* pCommandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void GenerateMipMaps(VulkanCommandPool* pCommandPool, VulkanDevice* pDevice, VkImage image, VkFormat format, I32 texWidth, I32 texHeight, U32 numMipLevels);
	void TransitionImageLayout(VulkanCommandPool* pCommandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, U32 numMipLevels);
	void CopyBufferToImage(VulkanCommandPool* pCommandPool, VkBuffer buffer, VkImage image, U32 width, U32 height);
	
	void BeginCommandBuffer(VkCommandBuffer commandBuffer);
	void EndCommandBuffer(VkCommandBuffer commandBuffer);
	void BeginRenderPass(VkCommandBuffer commandBuffer, VulkanRenderPass* pRenderPass, VkFramebuffer framebuffer, VkOffset2D offset, VkExtent2D extent, const std::vector<VkClearValue>& clearColors);
	void EndRenderPass(VkCommandBuffer commandBuffer);
	void DrawRenderableMesh(VkCommandBuffer commandBuffer, RenderableMesh* pMesh, VulkanPipeline* pPipeline, const std::vector<VkDescriptorSet> descriptorSets);
}