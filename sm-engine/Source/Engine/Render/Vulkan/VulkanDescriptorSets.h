#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanBuffer.h"
#include "Engine/Render/Vulkan/VulkanSampler.h"
#include "Engine/Render/Vulkan/VulkanTexture.h"
#include <vector>

class VulkanDescriptorSetLayout
{
public:
	VulkanDescriptorSetLayout();

	void PreInitAddLayoutBinding(U32 bindingIndex, U32 descriptorCount, VkDescriptorType descriptorType, VkShaderStageFlags shaderStages);
	void Init();
	void Destroy();

	VkDescriptorSetLayout m_layoutHandle;
	std::vector<VkDescriptorSetLayoutBinding> m_layoutBindings;
};

class VulkanDescriptorPool
{
public:
	VulkanDescriptorPool();

	void PreInitAddPoolSize(VkDescriptorType type, U32 count);
	void Init(U32 maxSets);
	void Reset();
	void Destroy();

	VkDescriptorSet AllocateSet(const VulkanDescriptorSetLayout& layout);
	std::vector<VkDescriptorSet> AllocateSets(const std::vector<VulkanDescriptorSetLayout>& layouts);
	std::vector<VkDescriptorSet> AllocateSets(const VulkanDescriptorSetLayout& layout, U32 numSets);

	VkDescriptorPool m_poolHandle;
	std::vector<VkDescriptorPoolSize> m_poolSizes;
	U32 m_maxSets;
};

class VulkanDescriptorSetWriter
{
public:
	struct WriteInfo
	{
		VkDescriptorType m_type;
		union
		{
			VkDescriptorBufferInfo m_bufferWriteInfo;
			VkDescriptorImageInfo m_imageWriteInfo;
		};
		VkDescriptorSet m_dstSet;
		U32 m_dstBinding;
		U32 m_dstArrayElement;
		U32 m_descriptorCount;
	};

	void AddUniformBufferWrite(VkDescriptorSet descriptorSet, const VulkanBuffer& buffer, U32 bufferOffset, U32 dstBinding, U32 dstArrayElement, U32 descriptorCount);
	void AddCombinedImageSamplerWrite(VkDescriptorSet descriptorSet, const VulkanTexture& texture, const VulkanSampler& sampler, VkImageLayout imageLayout, U32 dstBinding, U32 dstArrayElement, U32 descriptorCount);
	void AddSamplerWrite(VkDescriptorSet descriptorSet, const VulkanSampler& sampler, U32 dstBinding, U32 dstArrayElement, U32 descriptorCount);
	void AddSampledImageWrite(VkDescriptorSet descriptorSet, const VulkanTexture& texture, VkImageLayout imageLayout, U32 dstBinding, U32 dstArrayElement, U32 descriptorCount);

	void PerformWrites();

	std::vector<WriteInfo> m_writeInfos;
};
