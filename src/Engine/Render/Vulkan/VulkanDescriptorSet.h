#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"
#include <vector>

class VulkanDevice;
class VulkanBuffer;
class VulkanTexture;
class VulkanSampler;

class VulkanDescriptorSetLayout
{
public:
	VulkanDescriptorSetLayout(VulkanDevice* pDevice);
	~VulkanDescriptorSetLayout();

	void AddBinding(U32 bindingIndex, U32 count, VkDescriptorType type, VkShaderStageFlags stages);
	bool Setup();
	void Teardown();

	VkDescriptorSetLayout GetHandle();

private:
	VulkanDevice* m_pDevice;
	VkDescriptorSetLayout m_vkDescriptorSetLayout;
	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
};

class VulkanDescriptorPool
{
public:
	VulkanDescriptorPool(VulkanDevice* pDevice);
	~VulkanDescriptorPool();

	void AddPoolSize(VkDescriptorType type, U32 count);
	bool Setup(U32 maxSets);
	void Teardown();
	VkDescriptorPool GetHandle();

	std::vector<VkDescriptorSet> AllocateDescriptorSets(VulkanDescriptorSetLayout* pLayout, U32 numSets);

private:
	VulkanDevice* m_pDevice;
	VkDescriptorPool m_vkDescriptorPool;
	std::vector<VkDescriptorPoolSize> m_vkPoolSizes;
};

struct VulkanDescriptorSetWriteInfo
{
	VkDescriptorType m_vkDescriptorType;
	union
	{
		VkDescriptorBufferInfo m_vkBufferInfo;
		VkDescriptorImageInfo m_vkImageInfo;
	};
};

class VulkanDescriptorSetWriter
{
public:
	VulkanDescriptorSetWriter(VulkanDevice* pDevice);
	~VulkanDescriptorSetWriter();
	void Reset();
	void AddUniformBufferWrite(VulkanBuffer* pUniformBuffer, U32 dstBinding, U32 dstArrayElement);
	void AddImageSamplerWrite(VulkanTexture* pTexture, VulkanSampler* pSampler, VkImageLayout imageLayout, U32 dstBinding, U32 dstArrayElement);
	void WriteDescriptorSet(VkDescriptorSet vkDescriptorSet);

private:
	VulkanDevice* m_pDevice;
	std::vector<VkWriteDescriptorSet> m_vkDescriptorSetWrites;
	std::vector<VulkanDescriptorSetWriteInfo> m_vulkanDescriptorSetWriteInfos;
};