#include "Engine/Render/Vulkan/VulkanDescriptorSet.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Render/Vulkan/VulkanBuffer.h"
#include "Engine/Render/Vulkan/VulkanTexture.h"
#include "Engine/Render/Vulkan/VulkanSampler.h"
#include "Engine/Core/Assert.h"

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice* pDevice)
	:m_pDevice(pDevice)
	,m_vkDescriptorSetLayout(VK_NULL_HANDLE)
{
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
}

void VulkanDescriptorSetLayout::AddBinding(U32 bindingIndex, U32 count, VkDescriptorType type, VkShaderStageFlags stages)
{
	VkDescriptorSetLayoutBinding newBinding{};
	newBinding.binding = bindingIndex;
	newBinding.descriptorCount = count;
	newBinding.descriptorType = type;
	newBinding.stageFlags = stages;
	newBinding.pImmutableSamplers = nullptr;
	m_bindings.push_back(newBinding);
}

bool VulkanDescriptorSetLayout::Setup()
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = (U32)m_bindings.size();
	layoutInfo.pBindings = m_bindings.data();
		
	VULKAN_ASSERT(vkCreateDescriptorSetLayout(m_pDevice->GetHandle(), &layoutInfo, nullptr, &m_vkDescriptorSetLayout));
	return true;
}

void VulkanDescriptorSetLayout::Teardown()
{
	vkDestroyDescriptorSetLayout(m_pDevice->GetHandle(), m_vkDescriptorSetLayout, nullptr);
}

VkDescriptorSetLayout VulkanDescriptorSetLayout::GetHandle()
{
	return m_vkDescriptorSetLayout;
}

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice* pDevice)
	:m_pDevice(pDevice)
	,m_vkDescriptorPool(VK_NULL_HANDLE)
{
}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
}

void VulkanDescriptorPool::AddPoolSize(VkDescriptorType type, U32 count)
{
	VkDescriptorPoolSize newPoolSize{};
	newPoolSize.type = type;
	newPoolSize.descriptorCount = count;
	m_vkPoolSizes.push_back(newPoolSize);
}

bool VulkanDescriptorPool::Setup(U32 maxSets)
{
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = (U32)m_vkPoolSizes.size();
	poolInfo.pPoolSizes = m_vkPoolSizes.data();
	poolInfo.maxSets = maxSets;

	VULKAN_ASSERT(vkCreateDescriptorPool(m_pDevice->GetHandle(), &poolInfo, nullptr, &m_vkDescriptorPool));
	return true;
}

void VulkanDescriptorPool::Teardown()
{
	vkDestroyDescriptorPool(m_pDevice->GetHandle(), m_vkDescriptorPool, nullptr);
}

VkDescriptorPool VulkanDescriptorPool::GetHandle()
{
	return m_vkDescriptorPool;
}

std::vector<VkDescriptorSet> VulkanDescriptorPool::AllocateDescriptorSets(VulkanDescriptorSetLayout* pLayout, U32 numSets)
{
	std::vector<VkDescriptorSetLayout> layouts(numSets, pLayout->GetHandle());

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = GetHandle();
	allocInfo.descriptorSetCount = numSets;
	allocInfo.pSetLayouts = layouts.data();

	std::vector<VkDescriptorSet> descriptorSets;
	descriptorSets.resize(numSets);
	VULKAN_ASSERT(vkAllocateDescriptorSets(m_pDevice->GetHandle(), &allocInfo, descriptorSets.data()));

	return descriptorSets;
}

VulkanDescriptorSetWriter::VulkanDescriptorSetWriter(VulkanDevice* pDevice)
	:m_pDevice(pDevice)
{
}

VulkanDescriptorSetWriter::~VulkanDescriptorSetWriter()
{
}

void VulkanDescriptorSetWriter::Reset()
{
	m_vkDescriptorSetWrites.clear();
	m_vulkanDescriptorSetWriteInfos.clear();
}

void VulkanDescriptorSetWriter::AddUniformBufferWrite(VulkanBuffer* pUniformBuffer, U32 dstBinding, U32 dstArrayElement)
{
	VulkanDescriptorSetWriteInfo writeInfo{};
	writeInfo.m_vkDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeInfo.m_vkBufferInfo.buffer = pUniformBuffer->GetHandle();
	writeInfo.m_vkBufferInfo.offset = 0;
	writeInfo.m_vkBufferInfo.range = pUniformBuffer->GetSize();
	m_vulkanDescriptorSetWriteInfos.push_back(writeInfo);

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstBinding = dstBinding;
	descriptorWrite.dstArrayElement = dstArrayElement;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	m_vkDescriptorSetWrites.push_back(descriptorWrite);
}

void VulkanDescriptorSetWriter::AddImageSamplerWrite(VulkanTexture* pTexture, VulkanSampler* pSampler, VkImageLayout imageLayout, U32 dstBinding, U32 dstArrayElement)
{
	VulkanDescriptorSetWriteInfo writeInfo{};
	writeInfo.m_vkDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeInfo.m_vkImageInfo.imageView = pTexture->GetImageView();
	writeInfo.m_vkImageInfo.sampler = pSampler->GetHandle();
	writeInfo.m_vkImageInfo.imageLayout = imageLayout;
	m_vulkanDescriptorSetWriteInfos.push_back(writeInfo);

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstBinding = dstBinding;
	descriptorWrite.dstArrayElement = dstArrayElement;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	m_vkDescriptorSetWrites.push_back(descriptorWrite);
}

void VulkanDescriptorSetWriter::WriteDescriptorSet(VkDescriptorSet vkDescriptorSet)
{
	// Finish setting up writes
	for (U32 i = 0; i < m_vkDescriptorSetWrites.size(); i++)
	{
		m_vkDescriptorSetWrites[i].dstSet = vkDescriptorSet;
		switch (m_vkDescriptorSetWrites[i].descriptorType)
		{
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: m_vkDescriptorSetWrites[i].pBufferInfo = &m_vulkanDescriptorSetWriteInfos[i].m_vkBufferInfo; break;
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: m_vkDescriptorSetWrites[i].pImageInfo = &m_vulkanDescriptorSetWriteInfos[i].m_vkImageInfo; break;
		}
	}

	vkUpdateDescriptorSets(m_pDevice->GetHandle(), (U32)m_vkDescriptorSetWrites.size(), m_vkDescriptorSetWrites.data(), 0, nullptr);
}