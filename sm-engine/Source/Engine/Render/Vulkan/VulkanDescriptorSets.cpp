#include "Engine/Render/Vulkan/VulkanDescriptorSets.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout()
	:m_layoutHandle(VK_NULL_HANDLE)
	,m_pDevice(nullptr)
{
}

void VulkanDescriptorSetLayout::PreInitAddLayoutBinding(U32 bindingIndex, U32 descriptorCount, VkDescriptorType descriptorType, VkShaderStageFlagBits shaderStages)
{
	VkDescriptorSetLayoutBinding newBinding = {};
	newBinding.binding = bindingIndex;
	newBinding.descriptorCount = descriptorCount;
	newBinding.descriptorType = descriptorType;
	newBinding.stageFlags = shaderStages;
	newBinding.pImmutableSamplers = nullptr;
	m_layoutBindings.push_back(newBinding);
}

void VulkanDescriptorSetLayout::Init(const VulkanDevice* device)
{
	m_pDevice = device;

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = (U32)m_layoutBindings.size();
	layoutCreateInfo.pBindings = m_layoutBindings.data();

	SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(m_pDevice->m_deviceHandle, &layoutCreateInfo, nullptr, &m_layoutHandle));
}

void VulkanDescriptorSetLayout::Destroy()
{
	vkDestroyDescriptorSetLayout(m_pDevice->m_deviceHandle, m_layoutHandle, nullptr);
}

VulkanDescriptorPool::VulkanDescriptorPool()
	:m_pDevice(nullptr)
	,m_poolHandle(VK_NULL_HANDLE)
	,m_maxSets(0)
{
}

void VulkanDescriptorPool::PreInitAddPoolSize(VkDescriptorType type, U32 count)
{
	VkDescriptorPoolSize poolSize = {};
	poolSize.type = type;
	poolSize.descriptorCount = count;
	m_poolSizes.push_back(poolSize);
}

void VulkanDescriptorPool::Init(const VulkanDevice* pDevice, U32 maxSets)
{
	SM_ASSERT(pDevice != nullptr);
	m_pDevice = pDevice;
	m_maxSets = maxSets;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = (U32)m_poolSizes.size();
	createInfo.pPoolSizes = m_poolSizes.data();
	createInfo.maxSets = m_maxSets;

	SM_VULKAN_ASSERT(vkCreateDescriptorPool(m_pDevice->m_deviceHandle, &createInfo, nullptr, &m_poolHandle));
}

void VulkanDescriptorPool::Reset(VkDescriptorPoolResetFlags flags)
{
	vkResetDescriptorPool(m_pDevice->m_deviceHandle, m_poolHandle, flags);
}

void VulkanDescriptorPool::Destroy()
{
	vkDestroyDescriptorPool(m_pDevice->m_deviceHandle, m_poolHandle, nullptr);
}

VkDescriptorSet VulkanDescriptorPool::AllocateSet(VulkanDescriptorSetLayout& layout)
{
	std::vector<VulkanDescriptorSetLayout> layouts(1, layout);
	std::vector<VkDescriptorSet> ds = AllocateSets(layouts);
	return ds[0];
}

std::vector<VkDescriptorSet> VulkanDescriptorPool::AllocateSets(const std::vector<VulkanDescriptorSetLayout> layouts)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_poolHandle;
	allocInfo.descriptorSetCount = (U32)layouts.size();

	std::vector<VkDescriptorSetLayout> vkLayouts(layouts.size());
	for (I32 i = 0; i < vkLayouts.size(); i++)
	{
		vkLayouts[i] = layouts[i].m_layoutHandle;
	}
	allocInfo.pSetLayouts = vkLayouts.data();

	std::vector<VkDescriptorSet> vkDescriptorSets(layouts.size());
	SM_VULKAN_ASSERT(vkAllocateDescriptorSets(m_pDevice->m_deviceHandle, &allocInfo, vkDescriptorSets.data()));

	return vkDescriptorSets;
}

void VulkanDescriptorSetWriter::AddUniformBufferWrite(VkDescriptorSet descriptorSet, const VulkanBuffer& buffer, U32 bufferOffset, U32 dstBinding, U32 dstArrayElement, U32 descriptorCount)
{
	WriteInfo writeInfo;
	writeInfo.m_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeInfo.m_bufferWriteInfo.buffer = buffer.m_bufferHandle;
	writeInfo.m_bufferWriteInfo.offset = bufferOffset;
	writeInfo.m_bufferWriteInfo.range = buffer.m_size;
	writeInfo.m_dstBinding = dstBinding;
	writeInfo.m_dstSet = descriptorSet;
	writeInfo.m_dstArrayElement = dstArrayElement;
	writeInfo.m_descriptorCount = descriptorCount;
	m_writeInfos.push_back(writeInfo);
}

void VulkanDescriptorSetWriter::AddCombinedImageSamplerWrite(VkDescriptorSet descriptorSet, const VulkanTexture& texture, const VulkanSampler& sampler, VkImageLayout imageLayout, U32 dstBinding, U32 dstArrayElement, U32 descriptorCount)
{
	WriteInfo writeInfo;
	writeInfo.m_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeInfo.m_imageWriteInfo.imageView = texture.m_imageView;
	writeInfo.m_imageWriteInfo.sampler = sampler.m_samplerHandle;
	writeInfo.m_imageWriteInfo.imageLayout = imageLayout;
	writeInfo.m_dstBinding = dstBinding;
	writeInfo.m_dstSet = descriptorSet;
	writeInfo.m_dstArrayElement = dstArrayElement;
	writeInfo.m_descriptorCount = descriptorCount;
	m_writeInfos.push_back(writeInfo);
}

void VulkanDescriptorSetWriter::AddSamplerWrite(VkDescriptorSet descriptorSet, const VulkanSampler& sampler, U32 dstBinding, U32 dstArrayElement, U32 descriptorCount)
{
	WriteInfo writeInfo;
	writeInfo.m_type = VK_DESCRIPTOR_TYPE_SAMPLER;
	writeInfo.m_imageWriteInfo.sampler = sampler.m_samplerHandle;
	writeInfo.m_dstBinding = dstBinding;
	writeInfo.m_dstSet = descriptorSet;
	writeInfo.m_dstArrayElement = dstArrayElement;
	writeInfo.m_descriptorCount = descriptorCount;
	m_writeInfos.push_back(writeInfo);
}

void VulkanDescriptorSetWriter::AddSampledImageWrite(VkDescriptorSet descriptorSet, const VulkanTexture& texture, VkImageLayout imageLayout, U32 dstBinding, U32 dstArrayElement, U32 descriptorCount)
{
	WriteInfo writeInfo;
	writeInfo.m_type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writeInfo.m_imageWriteInfo.imageView = texture.m_imageView;
	writeInfo.m_imageWriteInfo.imageLayout = imageLayout;
	writeInfo.m_dstBinding = dstBinding;
	writeInfo.m_dstSet = descriptorSet;
	writeInfo.m_dstArrayElement = dstArrayElement;
	writeInfo.m_descriptorCount = descriptorCount;
	m_writeInfos.push_back(writeInfo);
}

void VulkanDescriptorSetWriter::PerformWrites(const VulkanDevice* pDevice)
{
	std::vector<VkWriteDescriptorSet> vkWrites(m_writeInfos.size());
	for (int i = 0; i < (int)m_writeInfos.size(); i++)
	{
		vkWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkWrites[i].pNext = nullptr;
		vkWrites[i].dstSet = m_writeInfos[i].m_dstSet;
		vkWrites[i].dstBinding = m_writeInfos[i].m_dstBinding;
		vkWrites[i].dstArrayElement = m_writeInfos[i].m_dstArrayElement;
		vkWrites[i].descriptorCount = m_writeInfos[i].m_descriptorCount;
		vkWrites[i].descriptorType = m_writeInfos[i].m_type;
		switch (m_writeInfos[i].m_type)
		{
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:             vkWrites[i].pBufferInfo = &m_writeInfos[i].m_bufferWriteInfo; break;
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:     vkWrites[i].pImageInfo = &m_writeInfos[i].m_imageWriteInfo; break;
			case VK_DESCRIPTOR_TYPE_SAMPLER:                    vkWrites[i].pImageInfo = &m_writeInfos[i].m_imageWriteInfo; break;
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:              vkWrites[i].pImageInfo = &m_writeInfos[i].m_imageWriteInfo; break;
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
			case VK_DESCRIPTOR_TYPE_MUTABLE_VALVE:
			case VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM:
			case VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM:
			case VK_DESCRIPTOR_TYPE_MAX_ENUM:
			default:
				SM_ERROR_MSG("Trying to set up a descriptor set write that isn't supported yet\n");
				break;
		}
	}

	vkUpdateDescriptorSets(pDevice->m_deviceHandle, (U32)vkWrites.size(), vkWrites.data(), 0, nullptr);
}
